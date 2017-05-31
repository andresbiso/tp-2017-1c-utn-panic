#! /bin/sh

cd ../..

#Bajo e instalo las commons
echo "Instalando las commons..."

git clone https://github.com/sisoputnfrba/so-commons-library.git
cd ~/so-commons-library
sudo make install
utnso
echo "Se instalaron las commons"

cd ..

#Bajo e instalo el parser de ansisop
echo "Instalando el parser de ansisop..."

git clone https://github.com/sisoputnfrba/ansisop-parser.git
cd ~/ansisop-parser/parser
sudo make all
utnso
sudo make install
utnso
echo "Se instalo el parser de ansisop"

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

exec bash

echo "Fin compilacion"

echo "Deploy finalizado exitosamente"
