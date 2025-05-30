# ScopeMux API

A FastAPI backend for the ScopeMux project with PostgreSQL, Redis, Celery, and Celery Beat.

## Project Structure

```
server/
├── alembic/                 # Database migrations
├── app/                     # Main application
│   ├── api/                 # API endpoints
│   │   └── v1/              # API version 1
│   │       └── endpoints/   # API endpoint modules
│   ├── core/                # Core modules (config, security)
│   ├── crud/                # CRUD operations
│   ├── db/                  # Database setup
│   ├── models/              # SQLAlchemy models
│   ├── schemas/             # Pydantic schemas
│   ├── services/            # Business logic
│   ├── main.py              # FastAPI application instance
│   └── worker.py            # Celery configuration
├── .env                     # Environment variables (not in git)
├── alembic.ini              # Alembic configuration
├── docker-compose.yml       # Docker Compose configuration
├── Dockerfile               # Dockerfile for FastAPI application
├── Dockerfile-celery        # Dockerfile for Celery worker
├── Dockerfile-celerybeat    # Dockerfile for Celery Beat
├── manage.py                # Command-line interface
├── README.md                # Project documentation
└── requirements.txt         # Python dependencies
```

## Getting Started

### Prerequisites

- Docker and Docker Compose

### Environment Variables

Create a `.env` file in the root directory with the following variables:

```
# PostgreSQL
POSTGRES_SERVER=db
POSTGRES_USER=postgres
POSTGRES_PASSWORD=postgres
POSTGRES_DB=app
DATABASE_URL=postgresql://postgres:postgres@db:5432/app

# Celery
CELERY_BROKER_URL=redis://redis:6379/0
CELERY_RESULT_BACKEND=redis://redis:6379/0

# API Settings
API_V1_STR=/api/v1
PROJECT_NAME=ScopeMux API

# Security
# Generate with: openssl rand -hex 32
SECRET_KEY=your_secret_key_here
ALGORITHM=HS256
ACCESS_TOKEN_EXPIRE_MINUTES=30
```

### Running the Application

```bash
# Start all services
docker-compose up -d

# Run database migrations
docker-compose exec api python manage.py db_upgrade

# Create a superuser (optional)
docker-compose exec api python manage.py create_superuser admin admin@example.com password
```

## Development

### Management Commands

The application includes a command-line interface (CLI) for common tasks:

```bash
# Start development server
python manage.py run

# Start an IPython shell with application context
python manage.py shell

# Database migrations
python manage.py db_migrate "migration message"
python manage.py db_upgrade
python manage.py db_downgrade

# Run Celery worker
python manage.py celery_worker

# Run Celery beat scheduler
python manage.py celery_beat
```

### API Documentation

When the application is running, you can access the API documentation at:

- Swagger UI: http://localhost:8000/docs
- ReDoc: http://localhost:8000/redoc

## License

This project is licensed under the MIT License - see the LICENSE file for details.
