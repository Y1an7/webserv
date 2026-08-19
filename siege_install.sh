#!/bin/bash

cd ~
wget -q -c http://download.joedog.org/siege/siege-4.0.7.tar.gz
tar -zxf siege-4.0.7.tar.gz
cd siege-4.0.7
sed -i 's/int strcasecmp();/\/\/ int strcasecmp();/g' src/setup.h
./configure --prefix=$HOME/.local
make CFLAGS="-g -O2 -std=gnu89 -w"
make install

cd ~
rm -f siege-4.0.7.tar.gz

if ! grep -q 'export PATH="$HOME/.local/bin:$PATH"' ~/.zshrc 2>/dev/null; then
	echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.zshrc
fi

if ! grep -q 'export PATH="$HOME/.local/bin:$PATH"' ~/.bashrc 2>/dev/null; then
	echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc
fi

echo "Siege installation successful"