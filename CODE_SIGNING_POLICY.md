# Code Signing Policy

**Project:** Precision Clock  
**Repository:** https://github.com/Colligatio-org/PrecisionClock  
**License:** GPL-3.0

---

## 1. Scope

This policy applies to the code signing of Windows binaries distributed through the official release channel of the Precision Clock project.

Signed file:

- `PrecisionClock.exe`

Not signed:

- Source code files (`main.cpp`, `app.rc`, `64x64.ico`)
- Linux AppImage and its source files
- Documentation (README, LICENSE)

## 2. Build and Signing Process

- All signed binaries are built from the public repository `Colligatio-org/PrecisionClock`. Source code and build scripts are publicly available.
- When a new version is released, a signing request is submitted through the SignPath CI/CD integration.
- The signing key is held in SignPath Foundation's Hardware Security Module (HSM). The project does not hold or have access to the private key.
- After signing, the binary is uploaded to the GitHub Release page for public download.

## 3. Roles

All signing operations are performed under the project's organization identity **`Colligatio-org`**.

| Role | Responsibility |
|---|---|
| Submitter | Initiates the signing request and uploads the binary for signing |
| Reviewer | Reviews the signing request and confirms the binary source |
| Approver | Gives final approval to trigger the signing process |

All roles are operated by the `Colligatio-org` organization account. No real personal information is associated with these roles. The GitHub organization account has multi-factor authentication enabled.

## 4. Privacy and Anonymity

- The project maintains an anonymous identity. Real names, schools, addresses, and other personally identifiable information are not used in any signing-related materials.
- All communications and records related to code signing avoid personally identifiable information.
- A dedicated anonymous email address is used for any required contact.

## 5. Compliance

- This project uses the OSI-approved **GPL-3.0** license, meeting the open-source requirements of SignPath Foundation.
- The project is fully free and open-source. It does not include any commercial licensing or dual-licensing terms.
- The project contains no malicious behavior, no advertisements, and no tracking.
- Binaries are provided free of charge to the public.
- Copyright of signed binaries remains with the Colligatio open-source project, and the GPL-3.0 license remains unchanged.

## 6. Updates

Any changes to signing scope, roles, or process will be reflected in this policy document.

---

*Last updated: 2026-10-07*
