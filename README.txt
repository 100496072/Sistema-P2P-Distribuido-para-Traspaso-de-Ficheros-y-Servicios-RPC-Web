COMPILACIÓN:
Previamente hace falta añadir la variable de entorno LIBRARY_PATH para que pueda compilar.

export LOG_RPC_IP=localhost
export LIBRARY_PATH= "Tu ruta"
rpcgen -MNa print_rpc.x
make

EJECUCIÓN:
Recomendado cada uno en un terminal por limpieza y añadir la variable de entorno LOG_RPC_IP para el funcionamiento del servidor socket.
1. ./onc_rpc_server
2. ./server -p 4200
3. python3 ws-time-service.py
4. python3 client.py -s localhost -p 4200