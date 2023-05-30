FROM ubuntu:22.04
# RUN sed -i 's/archive.ubuntu.com/mirrors.ustc.edu.cn/g' /etc/apt/sources.list && \
#     sed -i s@/security.ubuntu.com/@/mirrors.ustc.edu.cn/@g /etc/apt/sources.list && \
#     sed -i s@/ports.ubuntu.com/@/mirrors.ustc.edu.cn/@g /etc/apt/sources.list  && \
#     echo "force-unsafe-io" > /etc/dpkg/dpkg.cfg.d/02apt-speedup && \
#     echo "Acquire::http {No-Cache=True;};" > /etc/apt/apt.conf.d/no-cache && \
#     echo 'Acquire::Languages "none";' > /etc/apt/apt.conf.d/no-lang
RUN	apt update && \
apt -y install wget bzip2 xz-utils lib32z1 cmake vim 
#  ros-kinetic-cv-bridge ros-kinetic-image-transport  qtmultimedia5-dev ros-kinetic-cv-bridge  git && \
#     rm -r /var/lib/apt/lists/*
RUN wget https://ftp.denx.de/pub/u-boot/u-boot-2013.01.tar.bz2
RUN tar xvf u-boot-2013.01.tar.bz2
RUN rm -r u-boot-2013.01.tar.bz2
# RUN wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2

# RUN wget https://ftp.gnu.org/gnu/gcc/gcc-8.3.0/gcc-8.3.0.tar.gz

# RUN wget https://developer.arm.com/-/media/Files/downloads/gnu-a/10.3-2021.07/binrel/gcc-arm-10.3-2021.07-x86_64-arm-none-linux-gnueabi.tar.xz
# RUN tar xvf gcc-arm-10.3-2021.07-x86_64-arm-none-linux-gnueabi.tar.xz && rm -r gcc-arm-10.3-2021.07-x86_64-arm-none-linux-gnueabi.tar.xz

# RUN mv gcc-arm-10.3-2021.07-x86_64-arm-none-linux-gnueabi /opt/gcc-arm-10.3
# RUN ln -sf /opt/gcc-arm-10.3/bin/*  /usr/bin/

COPY ./gcc-4.6.4.tar.xz /
RUN tar xvf gcc-4.6.4.tar.xz && rm -r gcc-4.6.4.tar.xz

RUN mv gcc-4.6.4 /opt/gcc-4.6.4
RUN ln -sf /opt/gcc-4.6.4/bin/*  /usr/bin/

COPY ./make_file_doc/Makefile /u-boot-2013.01/



RUN cd /u-boot-2013.01 && cp  -rf  board/samsung/origen/  board/samsung/fs4412
RUN cd /u-boot-2013.01 && mv  board/samsung/fs4412/origen.c  board/samsung/fs4412/fs4412.c
RUN cd /u-boot-2013.01 && cp  include/configs/origen.h  include/configs/fs4412.h

COPY ./fs4412/Makefile /u-boot-2013.01/board/samsung/fs4412/Makefile
COPY ./fs4412/fs4412.h /u-boot-2013.01/include/configs/fs4412.h
COPY ./fs4412/boards.cfg /u-boot-2013.01/boards.cfg
# RUN cp /u-boot-2013.01/u-boot.bin /out/
# RUN cd /u-boot-2013.01 && make rpi_b
RUN cd /u-boot-2013.01 && make  fs4412_config
RUN  cd /u-boot-2013.01 && make

COPY ./encrypt/CodeSign4SecureBoot /u-boot-2013.01/CodeSign4SecureBoot

COPY ./encrypt/sdfuse_q /u-boot-2013.01/sdfuse_q


COPY ./make_file_doc/Makefilefinal  /u-boot-2013.01/Makefile

COPY ./make_file_doc/build.sh  /u-boot-2013.01/

COPY ./fs4412/LED/start.S /u-boot-2013.01/arch/arm/cpu/armv7/
COPY ./fs4412/UART/lowlevel_init.S /u-boot-2013.01/board/samsung/fs4412/lowlevel_init.S
COPY ./fs4412/Network/fs4412.h /u-boot-2013.01/include/configs/fs4412.h

COPY ./fs4412/Network/fs4412.c /u-boot-2013.01/board/samsung/fs4412/fs4412.c
RUN chmod a+x  /u-boot-2013.01/build.sh
RUN  cd /u-boot-2013.01 && ./build.sh


COPY ./fs4412/EMMC/movi.c /u-boot-2013.01/arch/arm/cpu/armv7/exynos/

COPY ./fs4412/EMMC/Makefile /u-boot-2013.01/arch/arm/cpu/armv7/exynos/Makefile

COPY ./fs4412/EMMC/fs4412.c  /u-boot-2013.01/board/samsung/fs4412/fs4412.c

COPY  ./fs4412/EMMC/command/*   /u-boot-2013.01/common/

COPY ./fs4412/EMMC/driver/mmc.c  /u-boot-2013.01/drivers/mmc/

COPY ./fs4412/EMMC/driver/s5p_mshc.c  /u-boot-2013.01/drivers/mmc/

COPY ./fs4412/EMMC/driver/include/* /u-boot-2013.01/include/

COPY ./fs4412/EMMC/driver/Makefile  /u-boot-2013.01/drivers/mmc/Makefile

COPY ./fs4412/EMMC/driver/fs4412.h /u-boot-2013.01/include/configs/fs4412.h

RUN  cd /u-boot-2013.01 && ./build.sh

COPY ./fs4412/POWER/pmic_s5m8767.c  /u-boot-2013.01/drivers/power/pmic/
COPY ./fs4412/POWER/Makefile /u-boot-2013.01/drivers/power/pmic/Makefile
COPY ./fs4412/POWER/pmic.h /u-boot-2013.01/include/power/pmic.h

COPY ./fs4412/POWER/fs4412.h  /u-boot-2013.01/include/configs/fs4412.h

COPY  ./fs4412/POWER/fs4412.c /u-boot-2013.01/board/samsung/fs4412/fs4412.c

COPY ./fs4412/POWER/drivermakefile/Makefile /u-boot-2013.01/drivers/power/Makefile

COPY ./fs4412/POWER/cpu_info.c /u-boot-2013.01/arch/arm/cpu/armv7/s5p-common/cpu_info.c

RUN  cd /u-boot-2013.01 && ./build.sh
# COPY ./fs4412/POWER/ /u-boot-2013.01/drivers/power/pmic/Makefile

COPY entrypoint.sh /entrypoint.sh 
RUN chmod a+x /entrypoint.sh
ENTRYPOINT ["/entrypoint.sh"]
#origen
# ENTRYPOINT ["entrypoint.sh" ]
