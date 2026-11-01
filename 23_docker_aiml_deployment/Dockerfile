# Small Python base image
FROM python:3.11-slim

# Do not write .pyc files, flush logs immediately
ENV PYTHONDONTWRITEBYTECODE=1 \
    PYTHONUNBUFFERED=1 \
    PORT=5000

WORKDIR /app

# Install dependencies first (better layer caching)
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Copy the application code
COPY app.py .

# Run as a non-root user for security
RUN useradd --create-home appuser
USER appuser

EXPOSE 5000
CMD ["python", "app.py"]
