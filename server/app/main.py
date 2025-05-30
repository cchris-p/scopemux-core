from fastapi import FastAPI, Depends
from sqlalchemy.orm import Session
from app.core.config import settings
from app.db import base as db_base
from app.db.base import engine

app = FastAPI(
    title=settings.PROJECT_NAME,
    openapi_url=f"{settings.API_V1_STR}/openapi.json"
)

# Example root endpoint
@app.get("/", tags=["Root"])
async def read_root():
    return {"message": f"Welcome to {settings.PROJECT_NAME}"}

# Example health check endpoint
@app.get("/health", tags=["Health"])
async def health_check(db: Session = Depends(db_base.get_db)):
    # Simple query to check DB connection
    try:
        db.execute("SELECT 1")
        return {"status": "healthy"}
    except Exception as e:
        return {"status": "unhealthy", "error": str(e)}

# Import and include API routers
from app.api.v1.endpoints import users
app.include_router(users.router, prefix=f"{settings.API_V1_STR}/users", tags=["users"])
