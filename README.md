# Proyecto Nº1

## Autores
* Fernando Álvarez: 20647921.
* Sebastián Arriagada: 16623959.
* Juan Martín Guzmán: 17625084.
* Rodrigo Rubio: 16640063.

## Principales decisiones de diseño
Para esta tarea se definió el struct CrmsFile con los siguientes atributos:
- pid: representa el id del archivo correspondiente.
- filename: representa el nombre del archivo correspondiente.
- vpn: virtual page number.
- offset: desplazamiento en memoria.
- mode: modo de lectura del archivo ('r','w').
- allocated

Para el resto de los elementos como Tabla de PCB's, Tabla de Páginas, Frame Bitmap y Frame no se definieron como structs, sino que se trabajó directamente con sus respectivos bytes de memoria, por lo que sus modificaciones quedan directamente plasmadas en el .bin

## Supuestos

