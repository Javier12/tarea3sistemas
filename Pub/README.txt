Este ejemplo es una adaptacion del tutorial incluido
(archivo "device drivers tutorial.pdf") y bajado de:
http://www.freesoftwaremagazine.com/articles/drivers_linux

---

Guia rapida:

Lo siguiente se debe realizar parados en
el directorio en donde se encuentra este README.txt

+ Compilacion (puede ser en modo usuario):
% make
...
% ls
... pub.ko ...

+ Instalacion (en modo root)

# mknod /dev/damas c 62 0
# mknod /dev/varones c 62 1
# chmod a+rw /dev/damas
# chmod a+rw /dev/varones
# insmod pub.ko
# dmesg | tail
...
[...........] Inserting pub module
#

+ Testing (en modo usuario preferentemente)

Ud. necesitara crear 4 shells independientes.  Luego
siga las instrucciones del enunciado de la tarea 3 de 2015-1

+ Desinstalar el modulo

# rmmod pub.ko
#
