FROM python:3.10-slim

# Copy the Python script into the container
COPY main.py /main.py

# Run the Python script
CMD "pip install kaggle-environments"
CMD ["python", "/main.py"]
