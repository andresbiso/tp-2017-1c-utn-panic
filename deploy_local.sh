#! /bin/sh

cd ../..

mv ~/workspace/tp-2017-1c-utn-panic/PANICommons/makefiles ~/workspace/tp-2017-1c-utn-panic/PANICommons/Debug

echo "Se copian los archivos de configuracion"
cp ~/workspace/tp-2017-1c-utn-panic/Kernel/src/config.txt ~/workspace/tp-2017-1c-utn-panic/Kernel/makefiles
cp ~/workspace/tp-2017-1c-utn-panic/CPU/src/config.txt ~/workspace/tp-2017-1c-utn-panic/CPU/makefiles
cp ~/workspace/tp-2017-1c-utn-panic/Memoria/src/config.txt ~/workspace/tp-2017-1c-utn-panic/Memoria/makefiles
cp ~/workspace/tp-2017-1c-utn-panic/FileSystem/src/config.txt ~/workspace/tp-2017-1c-utn-panic/FileSystem/makefiles
cp ~/workspace/tp-2017-1c-utn-panic/Consola/src/config.txt ~/workspace/tp-2017-1c-utn-panic/Consola/makefiles 

#Compilo los proyectos
echo "Compilando..."
cd ~/workspace/tp-2017-1c-utn-panic/PANICommons/Debug
make all
cd ~/workspace/tp-2017-1c-utn-panic/Kernel/makefiles
make all
cd ~/workspace/tp-2017-1c-utn-panic/CPU/makefiles
make all
cd ~/workspace/tp-2017-1c-utn-panic/Memoria/makefiles
make all
cd ~/workspace/tp-2017-1c-utn-panic/FileSystem/makefiles
make all
cd ~/workspace/tp-2017-1c-utn-panic/Consola/makefiles
make all

cd /home/utnso
echo "export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/workspace/tp-2017-1c-utn-panic/PANICommons/Debug" >> .bashrc

#Muevo archivos de prueba a carpeta personal
cp  -a ~/ansisop-parser/programas-ejemplo/evaluacion-final-esther/Scripts-Prueba/. ~
find ~/ansisop-parser/programas-ejemplo/ -name '*.ansisop' -exec cp -prv '{}' '/home/utnso/' ';' 
cp -a ~/workspace/tp-2017-1c-utn-panic/FileSystem/mnt/. ~


exec bash

echo "Deploy finalizado exitosamente"
