Purpose of this small quick and dirty project is to capture ethernet packets and then transmit
them on kafka. We use the following 5 Tuple: src ip, dst ip, protocol, src port, dst port as Kafka 
key for the configured topic.

The Kafka records are transmitted in binary format. The exact format will be documented in the future.


