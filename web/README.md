# 🐍 ESP32 Snake Game — Web Leaderboard

Backend web desarrollado en **Django** para visualizar los puntajes del juego Snake embebido en ESP32.
Los puntajes son enviados automáticamente desde el ESP32 via HTTP POST a Supabase al terminar cada partida,
y esta aplicación los recupera y presenta en un leaderboard interactivo.

**Desarrollado por:** Thiago Cabral & Franco Martínez  
**Materia:** Integración Tecnológica — Examen Final  
**Docente:** Romero, Facundo Ivan

---

## 📋 Requisitos previos

Antes de empezar, asegurate de tener instalado:
- Python 3.10 o superior
- Git

---

## 🚀 Instalación paso a paso

### 1. Clonar el repositorio

```bash
git clone https://github.com/FrancoMB1907/Esp32-Snake-game-Martinez-Franco-y-Cabral-Thiago.git
cd Esp32-Snake-game-Martinez-Franco-y-Cabral-Thiago
```

### 2. Entrar a la carpeta del backend web

```bash
cd web
```

### 3. Crear el entorno virtual

```bash
python -m venv venv
```

### 4. Activar el entorno virtual

En Windows (PowerShell):
```powershell
.\venv\Scripts\Activate.ps1
```

En Windows (CMD):
```cmd
venv\Scripts\activate.bat
```

En Mac/Linux:
```bash
source venv/bin/activate
```

> Cuando el entorno esté activo, vas a ver `(venv)` al inicio de la línea en la terminal.

### 5. Instalar las dependencias

```bash
pip install -r requirements.txt
```

---

## ⚙️ Configuración del archivo .env

**Este paso es obligatorio.** Sin el `.env` el proyecto no puede conectarse a la base de datos.

El archivo `.env` **no está en el repositorio** por seguridad (está en el `.gitignore`).
Tenés que crearlo manualmente dentro de la carpeta `web/`, al mismo nivel que `manage.py`.

### Creá el archivo `.env` con este contenido:

```env
DB_NAME=postgres
DB_USER=postgres.eppoqisvtsjtzdapdtld
DB_PASSWORD=ThiagoFrancoSnake
DB_HOST=aws-1-sa-east-1.pooler.supabase.com
DB_PORT=5432
```

### ¿Por qué usamos el Session Pooler?

La conexión directa de Supabase requiere IPv6, que no está disponible en la mayoría de
los ISP hogareños de Argentina. El Session Pooler (`aws-1-sa-east-1.pooler.supabase.com`)
actúa como proxy compatible con IPv4. Si usás el host directo de Supabase, la conexión va a fallar.

---

## 🗄️ Sincronización con la base de datos

Ejecutá las migraciones para que Django configure las tablas internas del sistema
(la tabla `partidas` ya existe en Supabase, Django no la toca gracias a `managed = False`):

```bash
python manage.py migrate
```

---

## ▶️ Correr el servidor

```bash
python manage.py runserver
```

Una vez iniciado, abrí el navegador en:

| URL | Descripción |
|-----|-------------|
| http://127.0.0.1:8000/ | Página de inicio del proyecto |
| http://127.0.0.1:8000/leaderboard/ | Tabla de puntajes con filtros y ordenamiento |

---

## 🏆 Funcionalidades del Leaderboard

- Tabla de puntajes ordenados por mayor a menor (por defecto)
- Filtros de ordenamiento: mayor puntaje, menor puntaje, más reciente, más antiguo
- Búsqueda por nombre de jugador
- Los datos se actualizan automáticamente al recargar la página

---