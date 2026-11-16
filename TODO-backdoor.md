# TODO

## Features to implement
- [ ] Credential exfiltration
    - [x] Implement client module. Ensure server info can be changed at any time
      - [x] Implement compile time value assignment for server host and port values and local file path for credentials
    - [ ] Develop server in new project
    - [x] Local exfil: Current implementation is to save information to a file on disk
      - [ ] If unchanged, it might be a good idea to implement a rootkit component to hide this file
    - [ ] remote_exfil() sleeps before attempting to reconnect, which can halt the entire process. Threading the function may fix this.

    - NOTE: monitor.c:998 --> The only place where `auth_password()` is called. This is within the `mm_answer_authpassword()` function. 
- [ ] Log erasure (Delete all log entries detailing successful login attempts that use backdoor functionality)
- [ ] Implement apt rules to block reinstallation of this SSH server
