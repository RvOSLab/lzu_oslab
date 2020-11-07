# lzu_oslab
A simple OS running on RISC-V for education

要想部署支持 RISC-V 的 QEMU，可以执行以下命令：

```shell
sudo apt update
sudo apt install git
# 因为现在是私有仓库，所以需要提前配置 ssh 的密钥
git clone git@github.com:LZU-OSLab/lzu_oslab.git
cd lzu_oslab
./setup.sh
```

