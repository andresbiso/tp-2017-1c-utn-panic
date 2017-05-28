#! /bin/sh

echo "export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/workspace/tp-2017-1c-utn-panic/PANICommons/makefiles" >> .bashrc

#Bajo e instalo las commons
echo "Instalando las commons..."

git clone https://github.com/sisoputnfrba/so-commons-library.git
cd ~/workspace/so-commons-library
sudo make install
utnso
echo "Se instalaron las commons"

#Bajo e instalo el parser de ansisop
echo "Instalando el parser de ansisop..."

git clone https://github.com/sisoputnfrba/ansisop-parser.git
cd ~/workspace/ansisop-parser
sudo make install
utnso
echo "Se instalo el parser de ansisop"

mv ~/workspace/PANICommons/makefiles ~/workspace/PANICommons/Debug

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
echo "Fin compilacion"


echo "Deploy finalizado exitosamente"
