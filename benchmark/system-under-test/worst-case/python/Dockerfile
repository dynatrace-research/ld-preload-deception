FROM python:3-bullseye

WORKDIR /app

COPY ./requirements.txt requirements.txt
RUN pip install -r requirements.txt

COPY ./app.py app.py

EXPOSE 5000

ENTRYPOINT [ "python3" ]
CMD [ "app.py" ]
