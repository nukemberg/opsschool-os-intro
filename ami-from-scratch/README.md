# AMI from scratch
In this exercise you will create an AMI (Amazon Machine Image) from scratch. AMI is a virtual machine image format used by EC2 instances and is a packaging of a disk image containing a comptible O/S. The kernel and optional initial ramdrive are usually provided seperately (AKI and ARI) and are registered in the AMI metadata. Unfortunately AWS does not allow registering kernel images without special permissions, so in order to run a custom kernel we will use a special AKI (kernel image) called `pvgrub` which is a special grub bootloader for hypervisors. `pvgrub` will then act like any other bootloader and load the kernel and ramdrive from the disk image itself allowing us to bundle our own kernel in the AMI.

## AMI types and instance types
There are 2 types of AMIs:
* EBS AMI - a snapshot of an EBS volume. A new volume will be provisioned from it to be the root device of the new instance
* Instance store AMI - an image file which saved on S3 and is deployed to a local disk of a host as a root device for the new instance

The main difference is that instance store AMIs are supported only by older instance types. You can create any image on any instance type but when testing the AMI you must use a compatible instance type. More information is available in the [EC2 documentation](https://docs.aws.amazon.com/AWSEC2/latest/UserGuide/ComponentsAMIs.html). If you are creating an instance store AMI, use an instance of type `m3.medium` or similar to check your work.


## Overview
The rough steps to create an AMI:
1. Create a raw disk image using `dd`, format it and mount it
1. Install Linux packages into the the new root using your distribution tooling
1. Create the devices files under `/dev`
1. Configure `/etc/fstab`
1. Install a suitable kernel
1. Create a config file for `pvgrub`
1. Configure the network interfaces
1. Create a non-root user with sudo permissions (optional)
1. Configure ssh, `cloud-init` and other AWS specific stuff (optional)
1. Bundle the image and upload it to S3

Make sure you are working on Linux, preferably Debian or Ubuntu since we will be installing Debian. It is highly recommended to use an EC2 instance since the upload to S3 will be much faster.

Most of the following commands need to be run as root (use sudo as appropriate or just work as root).

## Creating the disk image
### Instance store
To create a 2GB disk image we will use `dd`:
```
cd /mnt
dd if=/dev/zero of=ami-disk.img count=1024 bs=1M
```
Format the image to ext4 (note the label)
```
mkfs -t ext4 -L rootfs -F ami-disk.img
```
Mount the image
```
mkdir /mnt/image_root
mount -o loop ami-disk.img /mnt/image_root
```

### EBS
Create an EBS volume using `aws-cli` or the console with at least 1GB size (recommended < 10GB). Attach it to the instance you are working on and then (assuming you attached to `/dev/sdf` -> see bellow):

Format the volume to ext4 (note the label)
```
mkfs -t ext4 -L rootfs /dev/xvdf
```
Mount the volume
```
mkdir /mnt/image_root
mount /dev/xvdf /mnt/image_root
```

* A note on device names: Older virtualization use device names of physical devices like `/dev/sda` and EC2 still uses that on their API. On paravirtualized instances the kernel will see `/dev/sda` as `/dev/xvda`, `/dev/sdf` as `/dev/xvdf`, etc. 

## Installing Debian into the new root
We will use `debootstrap` - a Debian utility to bootstrap `dpkg` and `apt` into an empty root and install the base packages for Debian. Other distributions have equivalent tools.
```
debootstrap --arch amd64 stretch ./image_root
```

If `debootstrap` is not available on your machine you can install it with `apt install debootstrap`

Chroot into the new root and inspect it
```
chroot ./image_root
```

## Basic configurations
First of all, we need to create the basic devices in `/dev`
```
chroot ./image_root
mount -t proc proc /proc # we need this for MAKEDEV to work
apt update && apt install -y makedev
cd /dev
/sbin/MAKEDEV generic
/sbin/MAKEDEV zero
/sbin/MAKEDEV null
/sbin/MAKEDEV console
/sbin/MAKEDEV std
```
`MAKEDEV` is a shell script which creates devices using `mknod`, we are using it because it already knows the necessary device [major and minor numbers](http://books.gigatux.nl/mirror/solaris10examprep/0789734613/ch01lev1sec10.html) for various [standard devices](https://www.kernel.org/doc/Documentation/admin-guide/devices.txt).

Inspect the newly created devices:
```
ls -l /dev
```

Now that we have a device tree, let's configure the network. Edit `/etc/network/interfaces` on the new root (if you prefer, exit the chroot and edit from outside; remember the path will be `/mnt/image_root/etc/network/interfaces`). The new `interfaces` file should look something like this:
```
auto lo
iface lo inet loopback

auto eth0
iface eth0 inet dhcp
```

When booting, we need to make sure filesystems are automatically mounted. The `fstab` file should look something like:
```
LABEL=rootfs / ext3 defaults,discard 1 1
/dev/xvda2 /mnt ext3 defaults,nofail 0 0
/dev/xvda3 swap swap defaults 0 0
none /proc proc defaults 0 0
none /sys sysfs defaults 0 0
```

## Create a user and setup ssh
We could allow connecting as root, but that's frowned upon (I have my opinions, but let's go with the standard). We also need to setup sudo so that the user can manage the machine.

```
apt install -y sudo
adduser --disabled-password --add_extra_groups admin
echo 'admin ALL=(ALL) NOPASSWD:ALL' > /etc/sudoers.d/00_admin
```

To allow the user to login with an ssh key we need to integrate with EC2's metadata API. The metadata API is available from an EC2 instance on `http://169.254.169.254/latest/meta-data/` (try it!) and in the old days we used to fetch the SSH public key from it directly. Nowadays there's a utility designed to do just that (and much more) called [`could-init`](https://cloudinit.readthedocs.io/en/latest/). In this exercise we won't use it and do things ourselves.

```
apt install -y ssh
rm /etc/ssh/ssh_host_* # remove the ssh host keys, see below
```
Edit `/etc/ssh/sshd_config` and make sure `PasswordAuthentication` is uncommented and set to `no`:
```
...
# To disable tunneled clear text passwords, change to no here!
PasswordAuthentication no
#PermitEmptyPasswords no
...
```
Or just run this command:
```
sed -i 's/^#\s*PasswordAuthentication yes/PasswordAuthentication no/' /etc/ssh/sshd_config
```

Every SSH server should have a unique host key; We can't generate that key up front in the image because it will be the same for all servers booted from that image. Instead, we will put a script in `/etc/prep-instance` which is run on boot to generate the host key and fetch the user key from the metadata API. Edit `/etc/prep-instance` and put the following:

```
#!/bin/bash -e
if [[ ! -f /etc/ssh/ssh_host_ed25519_key ]]; then
        echo "Generating SSH host key"
        dpkg-reconfigure openssh-server
fi

wget -O /tmp/ssh_key http://169.254.169.254/latest/meta-data/public-keys/0/openssh-key
test -d ~admin/.ssh || sudo -u admin mkdir -m 0755 ~admin/.ssh
if ! grep -q "$(cat /tmp/ssh_key)" ~admin/.ssh/authorized_keys; then
        cat /tmp/ssh_key >> ~admin/.ssh/authorized_keys
        chown admin:admin ~admin/.ssh/authorized_keys
        echo 'Installed admin user key'
fi
```

Don't forget to `chmod +x /etc/prep-instance`

Finally, since debian uses systemd we need to have systemd run our `prep-instance` file on boot. Copy the `systemd-prep-instance.service` in this repository to your image `/etc/systemd/system/prep-instance.service` and enable it with `systemctl enable prep-instance`

## Setup grub and the kernel
How does EC2 know where the kernel is and which kernel to load? The answer is that the AMI metadata refers to AKI (Amazon Kernel Image) which is created and registered independently from the AMI. Registering an AKI is limited to AWS partners, but fortunately EC2 provides a special kernel image: [pvgrub](https://wiki.xenproject.org/wiki/PvGrub2). _pvgrub_ is a boot loader - basically a minimal kernel who'se only job is to find and load the real kernel. It can look into filesystems and looks for kernels in a designated boot partition, usually mounted as `/boot`. 

[pvgrub AWS documentation](https://docs.aws.amazon.com/AWSEC2/latest/UserGuide/UserProvidedKernels.html)

Install a kernel:
```
apt install -y linux-image-amd64
```


The `/boot/grub/menu.lst` file is grub's configuration file and tells it which kernels are available, where and how to boot them (e.g. kernel parameters).
It should look like this (copy from here or from the file in the repo, adjust for your kernel - `ls /boot/` is your friend):
```
default         0
fallback        1
timeout         0

title           Debian GNU/Linux, kernel 4.9.0-8-amd64
root            (hd0)
kernel          /boot/vmlinuz-4.9.0-8-amd64 root=LABEL=rootfs ro
initrd          /boot/initrd.img-4.9.0-8-amd64

title           Debian GNU/Linux, kernel 4.9.0-8-amd64 (fallback)
root            (hd0,0)
kernel          /boot/vmlinuz-4.9.0-8-amd64 root=LABEL=roots ro
initrd          /boot/initrd.img-4.9.0-8-amd64
```

## Bundle the image
First let's unmount the image
```
umount ./image_root/proc
umount ./image_root
```

### Instance store: using EC2 AMI tools
We will bundle the image using AWS' EC2 AMI tools. Follow the instructions below, for more information see [AMI tools setup guide](https://docs.aws.amazon.com/AWSEC2/latest/UserGuide/set-up-ami-tools.html).

Download the tools into your development server:
```
wget https://s3.amazonaws.com/ec2-downloads/ec2-ami-tools.zip
unzip ec2-ami-tools.zip
export EC2_AMITOOL_HOME=$PWD/ec2-ami-tools-1.5.7 # or whatever version you extracted
export PATH=$EC2_AMITOOL_HOME/bin:$PATH
apt install ruby # the ami tools are written in ruby
```

Create an X509 certificate for the tools (AMIs are cryptographically signed):
```
openssl req -new -x509 -nodes -sha256 -days 365 -keyout private-key.pem -newkey rsa:2048 -outform PEM -out certificate.pem
```
And register the certificate with your AWS IAM user (if aws cli tools are on a different computer - e.g. your laptop - copy the certificate there):
```
aws iam upload-signing-certificate --user-name your-user-name --certificate-body file://path/to/certificate.pem
```

Now that we have the AMI tools installed, let's bundle and upload the image! Edit the following command with the proper parameters, you can find the pv-grub aki in the [pvgrub AWS documentation](https://docs.aws.amazon.com/AWSEC2/latest/UserGuide/UserProvidedKernels.html) (in _eu-west-1_ that would be `aki-dc9ed9af`)
```
mkdir ami
ec2-bundle-image -c certificate.pem -k private-key.pem -i ami-disk.img -d ./ami -r x86_64 --kernel PV-GRUB-AKI --user YOUR_ACCOUNT_ID
```
Upload the image to S3 (just click Y if you get a warning about the region):
```
ec2-upload-bundle -a AWS_ACCESS_KEY_ID -s AWS_SECRET_ACCESS_KEY -b S3_BUCKET -m ami/ami-disk.img.manifest.xml -d ami --region REGION
```

And finally register it as an AMI (using `awscli`) - you will get the new AMI id from it:
```
aws ec2 register-image --name IMAGE_NAME --image-location S3_BUCKET/ami-disk.img.manifest.xml --region REGION
```

### EBS volume: create a snapshot and register the image
The following is done with `awscli` (on your laptop). Create a snapshot of the EBS volume you used:
```
aws ec2 create-snapshot --region REGION --volume-id VOLUME-ID --description "Be nice, write something here"
```
Edit the following command with the proper parameters, you can find the pv-grub aki in the [pvgrub AWS documentation](https://docs.aws.amazon.com/AWSEC2/latest/UserGuide/UserProvidedKernels.html) (in _eu-west-1_ that would be `aki-dc9ed9af`).
Register the snapshot as an AMI (using the snapshot id from the previous step):
```
aws ec2 register-image --region REGION --architecture x86_64 --kernel-id PV-GRUB-AKI --name "image-name" --description "some doc string" --root-device-name "/dev/sda1" --block-device-mappings '[
    {
        "DeviceName": "/dev/sda1",
        "Ebs": {
            "SnapshotId": "SNAPSHOT-ID"
        }
    },
    {
        "DeviceName": "/dev/sdb",
        "VirtualName": "ephemeral0"
    },
    {
        "DeviceName": "/dev/sdc",
        "VirtualName": "ephemeral1"
    },
    {
        "DeviceName": "/dev/sdd",
        "VirtualName": "ephemeral2"
    },
    {
        "DeviceName": "/dev/sde",
        "VirtualName": "ephemeral3"
    }
]'
```

## How to check your work
Run an instance using this image to test it's working ok (make sure you are using a compatible instance type if using instance store AMI):
```
aws ec2 run-instances --image-id AMI-ID --key-name YOUR_KEY_NAME --region REGION
```
If you did everything right, you should be able to ssh into your new instance with the admin user and your ssh key!

If you got it wrong, you can remove the image with `aws ec2 deregister-image` (you still have cleanup S3 yourself).

If you've successfully ran an instance from the newly created AMI and ssh'd into it with the `admin` user successfully, you're done!

If you've booted the AMI and cannot login to your instance, use EC2 console to get the instance console output. Right click on the instance in the console, select _Instance Settings_ -> _Get System Log_
