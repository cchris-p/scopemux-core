#!/usr/bin/env python3
import typer
from typing import Optional
import os
import sys
import importlib
import subprocess
from pathlib import Path

app = typer.Typer(help="ScopeMux API management commands")

@app.command()
def shell():
    """Start an IPython shell with app context."""
    try:
        import IPython
        from app.db.base import SessionLocal
        from app.db.base import Base
        
        # Import models for easy access in shell
        model_files = Path("app/models").glob("*.py")
        models = {}
        for file in model_files:
            if file.stem != "__init__" and file.stem != "__pycache__":
                module_name = f"app.models.{file.stem}"
                try:
                    module = importlib.import_module(module_name)
                    for attr_name in dir(module):
                        attr = getattr(module, attr_name)
                        if isinstance(attr, type) and issubclass(attr, Base) and attr != Base:
                            models[attr_name] = attr
                except ImportError:
                    pass
        
        # Create DB session
        db = SessionLocal()
        
        # Create shell context with imported objects
        context = {
            "db": db,
            "Base": Base,
            **models
        }
        
        typer.echo("Starting IPython shell...")
        IPython.start_ipython(argv=[], user_ns=context)
    except ImportError:
        typer.echo("IPython is not installed. Please install it with pip install ipython.")
        return

@app.command()
def run(host: str = "0.0.0.0", port: int = 8000, reload: bool = True):
    """Run the development server."""
    reload_arg = "--reload" if reload else ""
    command = f"uvicorn app.main:app --host {host} --port {port} {reload_arg}"
    typer.echo(f"Running server: {command}")
    subprocess.run(command, shell=True)

@app.command()
def db_migrate(message: Optional[str] = None):
    """Generate a database migration."""
    cmd = ["alembic", "revision", "--autogenerate"]
    if message:
        cmd.extend(["-m", message])
    subprocess.run(cmd)

@app.command()
def db_upgrade(revision: str = "head"):
    """Upgrade the database to the specified revision."""
    subprocess.run(["alembic", "upgrade", revision])

@app.command()
def db_downgrade(revision: str = "-1"):
    """Downgrade the database to the specified revision."""
    subprocess.run(["alembic", "downgrade", revision])

@app.command()
def create_superuser(username: str, email: str, password: str):
    """Create a superuser."""
    try:
        from app.db.base import SessionLocal
        from app.models.user import User
        from app.core.security import get_password_hash
        
        db = SessionLocal()
        user = User(
            username=username,
            email=email,
            hashed_password=get_password_hash(password),
            is_superuser=True,
        )
        db.add(user)
        db.commit()
        db.refresh(user)
        typer.echo(f"Superuser {username} created successfully!")
    except ImportError:
        typer.echo("User model not found. Please implement the user model first.")
    except Exception as e:
        typer.echo(f"Error creating superuser: {e}")

@app.command()
def celery_worker(loglevel: str = "info", queue: str = "default"):
    """Run Celery worker."""
    cmd = f"celery -A app.worker.celery_app worker --loglevel={loglevel} -Q {queue}"
    typer.echo(f"Starting Celery worker: {cmd}")
    subprocess.run(cmd, shell=True)

@app.command()
def celery_beat(loglevel: str = "info"):
    """Run Celery beat scheduler."""
    cmd = f"celery -A app.worker.celery_app beat --loglevel={loglevel}"
    typer.echo(f"Starting Celery beat: {cmd}")
    subprocess.run(cmd, shell=True)

if __name__ == "__main__":
    app()
