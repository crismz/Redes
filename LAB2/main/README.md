# INFORME REDES LAB2 

## Preguntas 

### ¿Qué estrategias existen para poder implementar este mismo servidor pero con capacidad de atender múltiples clientes simultáneamente?

Encontramos que para implementar este tipo de funcionalidad tenemos 2 opciones

#### 1. Utilizar la libreria 'threading' de python
- Cada vez que se acepte un cliente, se abre un nuevo hilo el cual se encarga de las instrucciones y el hilo principal vuelve, para aceptar nuevos clientes.
  
#### 2. Utilizar la libreria 'multiprocessing' de python  
- La idea es la misma que con los hilos, pero con otra libreria


### ¿Qué diferencia hay si se corre el servidor desde la IP “localhost”, “127.0.0.1” o la ip “0.0.0.0”?

Las direcciones IP 127.0.0.1 y 0.0.0.0 son similares en el sentido de que estan reservadas. No identifican a un dispositivo en particular en una red como otras direcciones IP, en cambio, se definen para uso especial.

127.0.0.1 se define como la direccion IP de loopback. El mecanismo del loopback puede ser utilizado para correr una red de servicio en un host local sin necesidad de una interfaz de red fisica. Esto lo hace no accesible para otros dispositivos en la misma red. 

localhost es simplemente el nombre de host de 127.0.0.1, en IPv4.

0.0.0.0 puede tener distintos significados segun el contexto. En este caso, puede representar cualquier direccion IP en una misma red, esto significa que setear un servidor a escuchar en la IP 0.0.0.0 hace que acepte pedidos de cualquier dispositivo en esa red.

Al ejecutar el servidor en una computadora del laboratorio, conectarse como cliente desde otra solo es posible si el servidor se ejecuta desde 0.0.0.0, pues esta direccion es capaz de escuchar conexiones entrantes desde cualquier dispositivo en la red, al contrario de 127.0.0.1/localhost, que solo es accesible para el dispositivo desde el cual se ejecuta el servidor.


## Decisiones de diseño tomadas

### Estructura del servidor

El servidor esta armado sobre una clase, en la cual al crear el objeto Server se inicializa con los protocolos IPv4 y TCP, y se pone a la escucha en un addr y port asignados, logrado con las funciones listen y bind respectivamente. 

Una vez creado el objeto e iniciado, se llama al método serve para comenzar a servir y esperar las conexiones de los clientes. Se acepta la conexión del cliente, y con su socket y address se crea el objeto connection, si todavia hay lugar (es decir no hay MAX_CLIENTS conectados) entonces se pasa atender al cliente, sino se le manda un mensaje y se cierra la conexión. 

Cuando se atiende a un cliente se crea un nuevo hilo que tiene como target al método handle de connection, se marca al hilo como "Hilo Demonio" (esto se hace para que si el servidor se rompe, el cliente no pierda la conexión sin aviso alguno y termine de ejecutar el método handle tirando el error correspondiente). Luego se inicia el hilo que se encarga de manejar los pedidos del cliente, mientras que el hilo servidor sigue a la espera de nuevos clientes. Cuando se cierra la conexión que tenia el hilo del cliente sale del método handle y se termina el hilo.

Decidimos limitar la cantidad de conexiones a 5 a la vez. Si se desea cambiar la cantidad de clientes se debe cambiar en la variable MAX_CLIENTS en constants.py.

### Manejo de pedidos

Cuando se crea la conexión en server se inicializa con el socket del cliente, el directorio de archivos y una flag para mantener información del estado de conexión. La flag se usa para los casos en que sucede algo por lo que se debe cerrar la conexión y se esta dentro de un ciclo, entonces se hace una comprobación para ver si la conexión sigue en pie o no, en caso de que no, se sale del ciclo.

El método handle se encarga de atender al cliente hasta que se cierre la conexión. 

Lo primero que hace es recibir el mensaje, hay una comprobación para que el mensaje no sea extremadamente largo, que seria para los casos malintencionados. Esta comprobración se hace en base a la variable MAX_BYTES definida en constants.py.
Luego separa el mensaje por EOL, para separar por los diferentes comandos y comprueba que el mensaje haya terminado con un EOL. Esto lo hace chequeando que el ultimo elemento de la lista sea un string vacío ya que el split va a separar el comando del ultimo espacio del mensaje y por eso queda vacío. Un ejemplo: ['abc',''] = abc\r\n.split(EOL).
Antes de empezar a ejecutar chequea que no haya un \n en alguno de los comandos.

Y por último ejecuta los comandos (o el comando si es uno solo) llamando a la función execute_command que chequea que comando es (o si no es ninguno) y llamando a la función correspondiente para ejecutarlo. Tambien en esta función se revisa que los parámetros de los comandos sean la cantidad correcta y los tipos correctos.

### Funciones para comandos

#### send()
Envia el mensaje al cliente, lo hace con la función send del socket. Lo hacemos con un while ya que el send no garantiza que envía todo en la primera llamada, por lo tanto hay un ciclo que va iterando hasta mandar todo el mensaje.

#### get_file_listing()
Devuelve los archivos que están en directory. Usamos el método listdir de la libreria os para obtener los archivos.

#### get_metadata()
Primero chequeamos que el nombre del archivo pasado sea válido, viendo que no haya algún carácter de los que no estan en VALID_CHARS. Luego se crea el path al archivo con el método join del os, chequeamos que exista el archivo. Y por último se	usa el método stat de os para ver el tamaño del archivo para asi enviarselo al cliente.

#### get_slice()
Al igual que get_metadata chequea el nombre del archivo y que exista. Luego revisa que el offset y el size sean positivos, y que su suma no sobrepase el tamaño del archivo. Por ultimo abre el archivo para leerlo, busca según el offset y lee size bytes. A eso lo coloca en un buffer y lo envía al cliente.

#### quit()
Cierra la conexión con el cliente y setea la flag connected a False.


## Dificultades con las que nos encontramos

### server.py
La unica dificultad que tuvimos en el server.py, que mas bien fue una duda, fue que no sabiamos si los hilos al salir del método handle seguian ejecutandose en el serve o se terminaban. Si se seguian ejecutando iba a ser un problema porque se iba a tener cada vez mas hilos en el servidor atendiendo clientes, y por cada cliente que haya cerrado la conexión iriamos sumando cada vez mas hilos (procesos) que se ejecutan. 

Averiguamos un poco como funcionaba, y vimos que la misma libreria se encargaba de terminar el hilo cuando saliera del método target que tuviera.

### connection.py
1. En la función get_slice, no sabiamos muy bien como abrir el archivo para leerlo. Creiamos que habia un problema porque el test-big-file de demoraba demasiado (hasta que después confirmamos que no era cuestión nuestra, sino del test mismo). En busca de una solución probamos encodearlo a medida que leia con el parámetro encoding que tiene la función open, pero no nos funcionaba bien de esa forma. Luego hicimos que al abrir el archivo para leer se especifique que se lea en binario con el caracter 'b', seguia demorando el test-big-file pero funcionaba bien. Entonces decidimos dejar esa forma cuando nos enteramos de que era cuestión del test.

2. Para detectar los errores de BAD_EOL tuvimos problemas ya que al principio testeabamos con el telnet y pasabamos un '\r\n' para ir probando, entonces armamos el código para detectar los errores en base a eso. Pero cuando haciamos los tests de BAD_EOL no los pasaba, hasta que nos dimos cuenta que no era lo mismo pasarle desde el telnet como un string a mandarselo con un send desde el cliente. Entonces empezamos a hacer el código en base a como seria un BAD_EOL con el send ayudandonos con los tests. Nos costo pensar como seria para la separación de los comandos y que funcione bien, en un principio intentamos usar el método splitlines pero no funcionaba bien. Al final usamos el split y como dejaba un espacio vacío al final, lo usamos para comprobar que terminar con un EOL. Luego a cada comando le buscabamos que no tuviera un \n.

### tests

La mayoria de las dificultades que tuvimos, fueron principalmente con los tests, como que al probar casos similares a los tests utilizando telnet, nos funcinaba bien, pero los tests seguian tirando errores, un ejemplo de esto es lo menciona en el punto 2 de las dificultades del connection.py.

Otro problema que tuvimos con los tests fue especificamente con el test big_file, como no sabiamos que tardaba bastante cancelabamos el proceso creyendo que se habia trabado en algún ciclo, y como cuando lo cancelabamos tiraba un error en el método send del socket creiamos que el problema venia de ahí. Probamos bastante, hasta que nos dimos cuenta que si pasaba el test pero que demoraba bastante.

