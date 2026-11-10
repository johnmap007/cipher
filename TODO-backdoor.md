# TODO

## Features to implement
- [ ] Credential exfiltration
    - [ ] Implement client module. Ensure server info can be changed at any time
    - [ ] Develop server in new project

    - NOTE: monitor.c:998 --> The only place where `auth_password()` is called. This is within the `mm_answer_authpassword()` function. 
- [ ] Log erasure (Delete all log entries detailing successful login attempts that use backdoor functionality)
