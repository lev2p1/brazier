# Security Policy

## Supported Versions

| Version | Supported          |
| ------- | ------------------ |
| 1.x.x   |        Yes         |
| < 1.0   |        No          |

## Reporting a Vulnerability

**Please do not report security vulnerabilities through public issues.**

Instead, report them by email to: **hello@brazier.org**

### What to Include

When reporting a vulnerability, please include:

- Description of the vulnerability
- Steps to reproduce the issue
- Potential impact assessment
- Affected versions (if known)
- Suggested fix (if any)
- Proof-of-concept code (if possible)

### What to Expect

- **Initial response**: Within 48 hours
- **Status update**: Within 7 days
- **Fix timeline**: Depends on severity (typically 30-90 days)
- **Public disclosure**: After fix is released

## Disclosure Policy

We follow coordinated disclosure:

1. You report the issue privately by email
2. We confirm and investigate the vulnerability
3. We develop and test a fix
4. We release the patch
5. We publicly disclose with credit to the reporter (unless anonymity requested)

## Severity Classification

| Severity | Response Time | Fix Timeline |
| -------- | ------------- | ------------ |
| Critical | 24 hours     | 7 days      |
| High     | 48 hours     | 30 days     |
| Medium   | 7 days       | 60 days     |
| Low      | 14 days      | 90 days     |

## Scope

### In Scope

- Vulnerabilities in brazier source code
- Security issues in build configuration
- Dependency vulnerabilities (reported to us, we will update)

### Out of Scope

- Third-party dependency issues (report upstream)
- Issues requiring physical access to a user's system
- Social engineering attacks
- Theoretical vulnerabilities without proof of concept
- Denial of service issues (unless critical)

## Recognition

We appreciate security researchers and will credit reporters in:

- Security advisories (published on GitVerse)
- Release notes
- CHANGELOG.md

Credit can be anonymous if requested.

## License

brazier is licensed under LGPL-3.0-or-later.

---

Thank you for helping keep brazier and its users safe.