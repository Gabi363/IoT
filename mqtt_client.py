import json
import os
import paho.mqtt.client as mqtt
from datetime import datetime

BROKER_URL = "192.168.121.210"
BROKER_PORT = 1883
USERNAME = "user1"
PASSWORD = "user1"

# List of topics to subscribe to
TOPICS = [
    "user1/(mac_address)/temperature",
    "user1/(mac_address)/heart_rate",
    "user1/(mac_address)/spo2"
]

DATA_FILE = "sensor_data.json"


# Function to save data to a JSON file
def save_data_to_file(topic, value):
    """Save MQTT data to a JSON file."""
    sensor_data = {}

    # Load existing data if the file exists
    if os.path.exists(DATA_FILE):
        try:
            with open(DATA_FILE, "r") as file:
                sensor_data = json.load(file)
        except Exception as e:
            print(f"Error loading {DATA_FILE}: {e}")

    # Prepare a timestamp
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    data_entry = {"value": value, "timestamp": timestamp}

    # Save data based on topic
    if topic not in sensor_data:
        sensor_data[topic] = []
    sensor_data[topic].append(data_entry)

    # Write back to file
    try:
        with open(DATA_FILE, "w") as file:
            json.dump(sensor_data, file, indent=2)
        print(f"Saved: {topic} -> {value} at {timestamp}")
    except Exception as e:
        print(f"Error saving to {DATA_FILE}: {e}")


# Callback when the client connects to the broker
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT Broker!")
        # Subscribe to topics
        for topic in TOPICS:
            client.subscribe(topic)
            print(f"Subscribed to topic: {topic}")
    else:
        print(f"Failed to connect, return code {rc}")


# Callback when a message is received
def on_message(client, userdata, msg):
    try:
        # Debugowanie: wypisz temat i payload
        # print(f"DEBUG: Topic={msg.topic}, Payload={msg.payload.decode('utf-8')}")

        # Obsługa wiadomości (jak poprzednio)
        value = msg.payload.decode("utf-8")
        if "temperature" in msg.topic:
            print(f"Temperature data received: {value}")
        elif "heart_rate" in msg.topic:
            print(f"Heart rate data received: {value}")
        elif "spo2" in msg.topic:
            print(f"SpO2 data received: {value}")
        save_data_to_file(msg.topic, value)
    except Exception as e:
        print(f"Error processing message: {e}")


# Callback when there is an error
def on_error(client, userdata, rc):
    print(f"Connection error: {rc}")


def main():
    client = mqtt.Client()

    # Set username and password for the broker
    client.username_pw_set(USERNAME, PASSWORD)

    # Define callbacks
    client.on_connect = on_connect
    client.on_message = on_message

    try:
        # Connect to the broker
        client.connect(BROKER_URL, BROKER_PORT, keepalive=60)

        # Start the MQTT loop
        client.loop_forever()
    except Exception as e:
        print(f"An error occurred: {e}")


if __name__ == "__main__":
    main()