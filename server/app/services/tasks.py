from app.worker import celery_app
import logging

logger = logging.getLogger(__name__)

@celery_app.task
def example_task(task_param: str):
    """
    An example task that takes a parameter and logs it.
    
    Args:
        task_param: A string parameter for the task
    
    Returns:
        dict: Result dictionary
    """
    logger.info(f"Running example task with parameter: {task_param}")
    # Replace with actual task logic
    return {"status": "success", "result": f"Processed {task_param}"}

@celery_app.task
def example_periodic_task():
    """
    An example periodic task that is scheduled to run at regular intervals.
    
    Returns:
        dict: Result dictionary
    """
    logger.info("Running periodic task")
    # Replace with actual periodic task logic
    return {"status": "success", "timestamp": "periodic task executed"}
