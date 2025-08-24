all:
	gcc -o  onc_rpc_server onc_rpc_server.c print_rpc_svc.c print_rpc_xdr.c -lrt -lpthread -ldl -ltirpc -lm -g -I/usr/include/tirpc
	gcc -o  server server.c print_rpc_clnt.c print_rpc_xdr.c users.c lines.c -fPIC -lrt -lpthread -ldl -ltirpc -lm -g -I/usr/include/tirpc


clean:
	rm -f server onc_rpc_server 

clear:
	rm -f server onc_rpc_server 