"""
ZMQ networking abstraction library
"""

import os

# Version management - can be set by environment variable or default
__version__ = os.environ.get('PACKAGE_VERSION', '0.0.0-dev')

# Import main classes/functions here
from .network_node import Node
from .config_reader import load_config, get_node_topics, get_node_address, is_valid_topic

__all__ = [
    '__version__',
    'Node',
    'get_node_topics',
    'get_node_address',
    'is_valid_topic'
]