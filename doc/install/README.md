### Install from distro respositorys

# Deiban 13 Trixi
```
sudo apt install chr
```

# Ubuntu >= 24.10
```
sudo apt install chr
```

# Ubuntu 22.04 - 24.04 (unofficial)

See [PPA](https://launchpad.net/~chr-istoph/+archive/ubuntu/chr)

```
sudo apt install software-properties-common
sudo add-apt-repository ppa:chr-istoph/chr
sudo apt install chr
```


# Fedora and Rhel (unofficial)
```
dnf install -y 'dnf-command(copr)'
dnf copr enable -y qsx42/tuiwidgets
dnf install -y chr
```

# Gentoo (unofficial)
```
emerge -q dev-vcs/git pkgdev app-eselect/eselect-repository
mkdir -p /etc/portage/repos.conf
cat <<EOF > /etc/portage/repos.conf/eselect-repo.conf
[gentoo-chr]
location = /var/db/repos/gentoo-chr
auto-sync = yes
sync-type = git
sync-uri = https://github.com/istoph/gentoo-chr.git
EOF
emaint sync -r gentoo-chr
emerge -a app-editors/chr
```


