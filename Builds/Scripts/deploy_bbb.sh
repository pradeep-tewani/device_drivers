#!/bin/bash

# 1. Argument Validation
if [ -z "$1" ]; then
    echo "Usage: $0 /dev/<mmcblkX or sdX> [images_path]"
    echo "Example: sudo $0 /dev/sdb ./my_output_images"
    exit 1
fi

DISK="$1"
# Set IMAGE_PATH: Use $2 if provided, otherwise default to ./Images
IMAGE_PATH="${2:-../Images}"

# 2. Safety Checks
if [ ! -b "$DISK" ]; then
    echo "Error: Device $DISK not found or is not a block device."
    exit 1
fi

if [ ! -d "$IMAGE_PATH" ]; then
    echo "Error: Image directory '$IMAGE_PATH' not found."
    exit 1
fi

# 3. User Confirmation
echo "************************************************************"
echo "WARNING: ALL DATA ON $DISK WILL BE PERMANENTLY WIPED!"
echo "Image Source: $IMAGE_PATH"
echo "************************************************************"
read -p "Are you sure you want to continue? (Type 'y' to continue): " CONFIRM

if [[ "$CONFIRM" != "y" ]]; then
    echo "Operation aborted."
    exit 0
fi

# 4. Partitioning
echo "Unmounting and Partitioning $DISK..."
sudo umount ${DISK}* 2>/dev/null

sudo sfdisk "$DISK" <<EOF
label: dos
size=256M, type=c, bootable
size=2G, type=83
EOF

sudo partprobe "$DISK"
sleep 2

# 5. Determine Partition Names
if [[ "$DISK" == *"mmcblk"* ]]; then
    PART1="${DISK}p1"
    PART2="${DISK}p2"
else
    PART1="${DISK}1"
    PART2="${DISK}2"
fi

# 6. Formatting
echo "Formatting $PART1 (FAT32) and $PART2 (EXT4)..."
sudo mkfs.vfat -F 32 -n BOOT "$PART1" > /dev/null
sudo mkfs.ext4 -F -L ROOTFS "$PART2" > /dev/null

# 7. Mounting and Copying Data
MOUNT_BOOT=$(mktemp -d)
MOUNT_ROOT=$(mktemp -d)

echo "Mounting partitions..."
sudo mount "$PART1" "$MOUNT_BOOT"
sudo mount "$PART2" "$MOUNT_ROOT"

# Copy Boot Files
echo "Copying Boot Files to Partition 1..."
BOOT_FILES=("MLO" "u-boot.img" "uEnv.txt" "am335x-boneblack.dtb" "zImage")
for file in "${BOOT_FILES[@]}"; do
    if [ -f "$IMAGE_PATH/$file" ]; then
        sudo cp "$IMAGE_PATH/$file" "$MOUNT_BOOT/"
        echo "  -> Copied $file"
    else
        echo "  !! Warning: $file not found in $IMAGE_PATH"
    fi
done

# Extract RootFS
echo "Extracting RootFS to Partition 2 (this may take a minute)..."
if [ -f "$IMAGE_PATH/rootfs.tar" ]; then
    sudo tar -xf "$IMAGE_PATH/rootfs.tar" -C "$MOUNT_ROOT"
    echo "  -> Extraction complete."
else
    echo "  !! Error: rootfs.tar not found in $IMAGE_PATH"
fi

# 8. Cleanup
echo "Syncing and unmounting..."
sync
sudo umount "$MOUNT_BOOT"
sudo umount "$MOUNT_ROOT"
rmdir "$MOUNT_BOOT" "$MOUNT_ROOT"

echo "------------------------------------------------------------"
echo "SD Card is ready for BeagleBone Black!"
echo "------------------------------------------------------------"
