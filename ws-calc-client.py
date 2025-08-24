import zeep

wsdl = "http://localhost:8000/?wsdl"

client = zeep.Client(wsdl=wsdl)
print(client.service.obtener_fecha_hora())

output = client.service.obtener_fecha_hora()
x = output.split(" ")
print(x[0])
print(x[1])
print(output)

