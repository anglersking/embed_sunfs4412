docker rm -f uboot_build
docker build --no-cache -t uboot_build . 
docker run -dit --net=host -v $(pwd)/out:/out --name=uboot_build uboot_build
# docker rm -f uboot_build