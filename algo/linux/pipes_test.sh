#! /usr/bin/sh

while true
do
  read -p 'Please input a line of text: ' input
  echo ${input} > ~/mnt/box/in_pipe
  echo 'Read from out_pipe: '
  cat ~/mnt/box/out_pipe
  echo
  echo
done
