FROM python:3.8-bullseye

WORKDIR /app

COPY ./src/requirements.txt requirements.txt
RUN pip install -r requirements.txt

COPY ./src/app.py app.py

EXPOSE 5000

ENTRYPOINT [ "python3" ]
CMD [ "app.py" ]
