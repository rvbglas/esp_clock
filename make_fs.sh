#!/bin/sh

PORT=/dev/ttyUSB1

mkdir -p ./.data-release
cp -r ./data/* ./.data-release/

cat ui.yml | yq -c > ./.data-release/ui.json

find ./.data-release/ -name "*~" -type f -delete
find ./.data-release/web/ -type f -name "*.css" ! -name "*.min.*" -exec echo {} \; -exec uglifycss --output {}.min {} \; -exec rm {} \; -exec mv {}.min {} \;
find ./.data-release/web/ -type f -name "*.js" ! -name "*.min.*" ! -name "vfs_fonts*" -exec echo {} \; -exec uglifyjs -o {}.min {} \; -exec rm {} \; -exec mv {}.min {} \;
find ./.data-release/web -type f -exec gzip {} +

mklittlefs -c ./.data-release littlefs.img -d 5 -b 8192 -p 256 -s 0xfb000
esptool.py --port $PORT write_flash 0x300000 littlefs.img 
rm -r ./.data-release
