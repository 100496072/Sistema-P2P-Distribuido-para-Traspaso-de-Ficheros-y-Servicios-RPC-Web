from datetime import datetime
from wsgiref.simple_server import make_server
from spyne import Application, ServiceBase, Unicode, rpc
from spyne.protocol.soap import Soap11
from spyne.server.wsgi import WsgiApplication
import logging

# Configuración de logging
logging.basicConfig(level=logging.INFO)
logging.getLogger('spyne').setLevel(logging.WARNING)

class TiempoService(ServiceBase):
    @rpc(_returns=Unicode)
    def obtener_fecha_hora(ctx):
        return datetime.now().strftime("%d/%m/%Y %H:%M:%S")

app = Application(
    [TiempoService],
    tns='http://tests.python-zeep.org/',
    in_protocol=Soap11(validator='lxml'),
    out_protocol=Soap11()
)

if __name__ == '__main__':
    logging.info("Escuchando en http://127.0.0.1:8000 (WSDL: ?wsdl)")
    server = make_server('0.0.0.0', 8000, WsgiApplication(app))
    server.serve_forever()

