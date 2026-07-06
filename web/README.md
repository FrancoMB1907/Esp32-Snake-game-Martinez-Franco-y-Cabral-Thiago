#  ESP32 Snake Game - Web Backend (Leaderboard)

Este es el backend web desarrollado en **Django** para el proyecto del **Parcial 2 de Diseño de Sistemas**. El sistema se encarga de gestionar el procesamiento de datos, la lógica de la aplicación y la persistencia de las puntuaciones de las partidas (Leaderboard), conectándose de forma remota a una base de datos PostgreSQL alojada en **Supabase**.

---

##  Prerrequisitos

Antes de comenzar, asegúrate de tener instalado en tu entorno local:
* Python 3.10 o superior.
* Git.

---

##  Instalación y Configuración Local

Sigue estos pasos en orden para clonar el repositorio y levantar el entorno de desarrollo en tu máquina:

### 1. Clonar el repositorio y posicionarse en la carpeta
```bash
git clone <URL_DE_VUESTRO_REPOSITORIO>
cd Esp32-Snake-game-Martinez-Franco-y-Cabral-Thiago/web
```

### 2. Crear y activar el entorno virtual (`venv`)
En Windows (PowerShell):
```powershell
python -m venv venv
.\venv\Scripts\Activate.ps1
```

### 3. Instalar las dependencias del proyecto
Asegúrate de tener el entorno virtual activo (deberías ver el indicador `(venv)` al inicio de la línea de comandos en la terminal) e instala todas las librerías necesarias registradas en el archivo de requerimientos:
```powershell
pip install -r requirements.txt
```

---

##  Explicación Detallada de las Variables de Entorno (.env)

El proyecto utiliza la librería `python-dotenv` para cargar credenciales sensibles de forma dinámica al inicializar el entorno. Por motivos estrictos de seguridad, **el archivo `.env` original está incluido en el `.gitignore` y NUNCA se debe subir al repositorio público de GitHub**. Compartir contraseñas de bases de datos en internet expone la infraestructura a accesos maliciosos o alteraciones no autorizadas.

Para que el proyecto funcione en tu computadora local, debes crear un archivo de texto plano llamado exactamente **`.env`** en la raíz de la carpeta `web/` (al mismo nivel donde se encuentra el archivo `manage.py`). Puedes guiarte con la estructura del archivo de plantilla `.env.example`.

### Código para copiar y pegar en tu `.env`
Crea el archivo, copia el siguiente bloque de texto, pégalo dentro y completa los campos correspondientes:


DB_NAME=postgres
DB_USER=postgres.eppoqisvtsjtzdapdtld
DB_PASSWORD=ThiagoFrancoSnake
DB_HOST=aws-1-sa-east-1.pooler.supabase.com
DB_PORT=5432


###  Desglose Técnico de cada Variable:

* **`DB_NAME`**: Especifica el nombre de la base de datos lógica dentro del servidor remoto. Por defecto, en las instancias iniciales de Supabase se asigna el nombre `postgres`.
* **`DB_USER`**: Es el usuario de autenticación para la base de datos. **¡MUY IMPORTANTE!** Debido a que utilizamos el *Session Pooler* de Supabase para compatibilidad con redes IPv4, el usuario no es simplemente `postgres`; requiere obligatoriamente incluir el identificador único del proyecto adjunto (`postgres.eppoqisvtsjtzdapdtld`). Si se omite este sufijo, el Pooler rechazará la solicitud de conexión de forma inmediata.
* **`DB_PASSWORD`**: La contraseña maestra y secreta que definiste de forma manual al crear el proyecto en el panel de control de Supabase. Reemplaza el texto de marcador por tu clave real.
* **`DB_HOST`**: La dirección del servidor remoto al que se conectará Django. Aquí se declara la URL del pooler de sesiones (`aws-1-sa-east-1.pooler.supabase.com`). Esto es indispensable debido a que la conexión directa estándar de Supabase exige el uso de protocolos IPv6 nativos, los cuales no suelen estar disponibles en los proveedores de internet (ISP) hogareños de Argentina. El Pooler actúa como un proxy intermedio compatible con redes IPv4 tradicionales.
* **`DB_PORT`**: El puerto de red estructurado para la comunicación con instancias PostgreSQL. En la configuración regional asignada por la infraestructura de Supabase, el puerto establecido es el `5432`.

---

##  Sincronización de la Base de Datos (Migraciones)

Una vez configurado el archivo `.env` con las credenciales de acceso correctas, debes impactar la estructura de datos local en la nube. Django leerá los modelos y creará todas las tablas requeridas por el sistema base ejecutando el siguiente comando en la terminal:

```powershell
python manage.py migrate
```

###  Crear un Usuario Administrador (Superuser)
Si necesitas ingresar al entorno visual de administración nativo de Django para auditar, modificar o añadir registros directamente sobre las tablas de la base de datos, ejecuta:
```powershell
python manage.py createsuperuser
```
La consola te solicitará de forma secuencial un nombre de usuario, una dirección de correo electrónico y una contraseña de acceso. *Nota: Al escribir la contraseña en la terminal de Windows, los caracteres no se mostrarán en pantalla ni se moverá el cursor por razones estrictas de privacidad; tipea la clave normalmente y presiona Enter.*

---

##  Ejecución del Servidor de Desarrollo

Con las dependencias instaladas de forma correcta, las variables de entorno locales configuradas y las migraciones estructurales aplicadas sobre Supabase, ya puedes iniciar el servidor web de desarrollo local mediante el comando:

```powershell
python manage.py runserver
```

El entorno local quedará activo. Puedes validar su correcto funcionamiento interactuando con las siguientes direcciones desde tu navegador web:
* **Raíz del Servidor Local:** [http://127.0.0.1:8000/](http://127.0.0.1:8000/)
* **Panel de Administración Central:** [http://127.0.0.1:8000/admin/](http://127.0.0.1:8000/admin/)

---
 **Desarrollado por:** Thiago Cabral & Franco Martínez
```