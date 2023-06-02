#!/bin/bash

#
# Usage: prepend_image_info.sh in_file out_file
#

#
# Encrypted Image Information Data Structure
#       version:	         4 bytes
#       magic:                   2 bytes
#       padding:                 2 bytes
#       encrypted image size:    4 bytes
#       encrypted image offset:  4 bytes
#       padding:                48 bytes
#
# Note:
#   *) total size of Encrypted Image Information Data Structure is 64 bytes
#   *) encrypted image starts from offset 0x40 (64 B) with this script

#
# Functions
#

function be2le_2() {
  local s_b1=$(echo $1 | cut -b 3-4)
  local s_b2=$(echo $1 | cut -b 1-2)

  echo "${s_b1}${s_b2}"
  return 0
}

function be2le_4() {
  local s_b1=$(echo $1 | cut -b 7-8)
  local s_b2=$(echo $1 | cut -b 5-6)
  local s_b3=$(echo $1 | cut -b 3-4)
  local s_b4=$(echo $1 | cut -b 1-2)

  echo "${s_b1}${s_b2}${s_b3}${s_b4}"
  return 0
}

# Predefined variables

enc_img_info_ver=$(be2le_4 "01000000")
enc_img_info_magic=$(be2le_2 "e11d")

enc_img_offset=$(be2le_4 "00000040")

pad_2="0000"
pad_4="${pad_2}${pad_2}"
pad_8="${pad_4}${pad_4}"
pad_16="${pad_8}${pad_8}"
pad_48="${pad_16}${pad_16}${pad_16}"

#
# Main
#

# Parse command line arguments

if [ $# != "2" ]; then
    echo "Usage: gen_enc_img_info.sh in_file out_file"
    exit 1
fi

in_file=$1
out_file=$2

# Prepare data structure

len_in_file=$(stat -c %s $in_file)
len_hex=$(printf %08x $len_in_file)
len_s=$(be2le_4 "$len_hex")

# Generate encrypted image information data structure
echo "$enc_img_info_ver" "$enc_img_info_magic" "$pad_2" "${len_s}" \
	"$enc_img_offset" "$pad_48" | xxd -r -p > $out_file

# Padding to 1KiB
#dd if=/dev/zero bs=$(( 1024-64 )) count=1 >> $out_file

# Concatenate encrypted image
cat $in_file >> $out_file
