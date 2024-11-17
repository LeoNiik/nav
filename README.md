###  Description 

Shell utility to navigate through directories.\
Notes:\
This works only on bash right now.\
Still unusable under the root user.\
It uses fzf to search a directory to cd to.
nav is actually an alias in `~/.bashrc`.
To avoid bugs execute the `sudo ./install.sh` with the user you want the command for.
### Install

Make sure to install the dependency fzf

On Debian Based:

```bash
sudo apt install fzf gcc
```

To install the nav command:

```bash
sudo ./install.sh 
```

### Demo

[Screencast from 2024-11-14 19-06-30.webm](https://github.com/user-attachments/assets/f78c3109-d3d4-454a-a3a0-64478a17dadf)
