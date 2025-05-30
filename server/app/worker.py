from celery import Celery
from app.core.config import settings

celery_app = Celery(
    "worker",
    broker=settings.CELERY_BROKER_URL,
    backend=settings.CELERY_RESULT_BACKEND,
)

# Include tasks from different modules
celery_app.conf.imports = ["app.services.tasks"]

# Optional configuration
celery_app.conf.update(
    task_serializer="json",
    accept_content=["json"],
    result_serializer="json",
    timezone="UTC",
    enable_utc=True,
    worker_hijack_root_logger=False,
)

# Optional: Configure Celery Beat
celery_app.conf.beat_schedule = {
    "example-task-every-30-minutes": {
        "task": "app.services.tasks.example_periodic_task",
        "schedule": 1800.0,  # Execute every 30 minutes
    },
}
