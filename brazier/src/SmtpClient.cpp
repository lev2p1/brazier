/*
 * Copyright (c) 2026 Kirill Sergeev, Nikolay Sugonyako, Andrey Agarkov, Gleb Safyannikov
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * This file is part of brazier.
 *
 * brazier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * brazier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with brazier; if not, see <https://www.gnu.org/licenses/>.
 */

#include "../include/brazier/App/Http/Helpers/SmtpClient.hpp"
#include "../include/brazier/App/Http/Helpers/Code.hpp"
#include <algorithm>

brazier::SmtpClient::~SmtpClient() {
    std::ranges::fill(this->username, 0);
    std::ranges::fill(this->password, 0);
    this->username.clear();
    this->password.clear();
    this->username.shrink_to_fit();
    this->password.shrink_to_fit();
}

brazier::SmtpClient::SmtpClient()
    : io_ctx_(Engine::get_io_context()), ssl_ctx_(ssl::context::tls_client) {

    this->host = ENV::get("S_HOST");
    this->smtp_host = ENV::get("SMTP_HOST");
    this->port = ENV::get("SMTP_PORT");
    this->username = ENV::get("SMTP_USERNAME");
    this->password = ENV::get("SMTP_PASSWORD");
}

boost::asio::awaitable<int> brazier::SmtpClient::send(const std::string& recipient, const std::string& message, const std::string& sender_name ) {
    try {
        tcp::resolver resolver(io_ctx_);
        auto endpoints = resolver.resolve(this->smtp_host, this->port);

        ssl::stream<tcp::socket> socket(io_ctx_, ssl_ctx_);

        if (!SSL_set_tlsext_host_name(socket.native_handle(), this->smtp_host.c_str())) {
            throw boost::system::system_error(
                boost::system::error_code(static_cast<int>(::ERR_get_error()),
                boost::asio::error::get_ssl_category()));
        }

        co_await net::async_connect(socket.next_layer(), endpoints);
        co_await socket.async_handshake(ssl::stream_base::client, net::use_awaitable);

        co_await read_response(socket, 220);

        co_await write_line(socket, "EHLO " + this->host);
        co_await read_response(socket, 250);

        co_await write_line(socket, "AUTH PLAIN");
        co_await read_response(socket, 334);

        std::string auth_string = '\0' + this->username + '\0' + this->password;
        co_await write_line(socket, Code::base64_encode(auth_string));
        co_await read_response(socket, 235);

        co_await write_line(socket, "MAIL FROM:<" + this->username + ">");
        co_await read_response(socket, 250);

        co_await write_line(socket, "RCPT TO:<" + recipient + ">");
        co_await read_response(socket, 250);

        co_await write_line(socket, "DATA");
        co_await read_response(socket, 354); // Start mail input

        std::string email_data =
            "From: " + (sender_name.size() > 0 ? sender_name + " <" + this->username + ">" : this->username) + "\r\n"
            "To: " + recipient + "\r\n"
            "Subject: Notification\r\n"
            "MIME-Version: 1.0\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "\r\n" +
            message + "\r\n.\r\n";

        co_await write_line(socket, email_data);
        co_await read_response(socket, 250);

        co_await write_line(socket, "QUIT");

        co_return 0;
    }
    catch (const std::exception& e) {
        const std::string masked_username = this->username.substr(0, 3) + "***";
        const std::string masked_password = this->password.substr(0, 3) + "***";

        std::string msg = e.what();

        size_t pos = msg.find(this->username);
        while (pos != std::string::npos) {
            msg.replace(pos, this->username.length(), masked_username);
            pos = msg.find(this->username, pos + this->username.length());
        }

        pos = msg.find(this->password);
        while (pos != std::string::npos) {
            msg.replace(pos, this->password.length(), masked_password);
            pos = msg.find(this->password, pos + this->password.length());
        }

        Logger::log("SMTP Error: " + msg, "ERROR");
        co_return -1;
    }
}

boost::asio::awaitable<void> brazier::SmtpClient::read_response(
    ssl::stream<tcp::socket>& socket,
    const int expected_code
) {
    net::streambuf buf;
    std::string line;

    do {
        co_await net::async_read_until(socket, buf, "\r\n", net::use_awaitable);
        std::istream is(&buf);
        std::getline(is, line);

        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        /*
         * Uncomment this to see log responses from server.
        */
        //Logger::log("SMTP Response: " + line, "INFO");

        if (line.size() < 3) {
            throw std::runtime_error("Invalid SMTP response: " + line);
        }

        const int code = std::stoi(line.substr(0, 3));

        if (line.length() > 3 && line[3] == ' ') {
            if (code != expected_code) {
                throw std::runtime_error(
                    "SMTP Error: Expected " + std::to_string(expected_code) +
                    ", got " + line
                );
            }
            break;
        }

    } while (true);

    co_return;
}

boost::asio::awaitable<void> brazier::SmtpClient::write_line(
    ssl::stream<tcp::socket>& socket,
    const std::string& line
) {
    co_await net::async_write(socket, net::buffer(line + "\r\n"), net::use_awaitable);
}