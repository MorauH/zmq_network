import json
import os
config = None


def load_config(config_path: str):
    global config
    with open(config_path, "r") as f:
        config = json.load(f)


def get_node_address(node: str) -> str:
    global config
    try:
        data = config["nodes"][node]
        is_ipc = data["is_ipc"]
        address = data["address"]
        if is_ipc:
            return "ipc://" + address
        else:
            port = data["port"]
            return "tcp://" + address + ":" + str(port)
    except KeyError:
        raise Exception("Node not found in config")


# check if topic is valid for node, returns true if is valid
def is_valid_topic(node: str, topic: bytes) -> bool:
    valid_topics = get_node_topics(node)
    return (topic in valid_topics)


# return node topic as List of bytes b"topic"
def get_node_topics(node: str) -> list[bytes]:
    global config
    try:
        topics = config["nodes"][node]["topics"]
        return [bytes(t, "utf-8") for t in topics]
    except KeyError:
        raise Exception("Node not found in config")

