from linuxcnc_arduinoconnector.ArduinoConnector import ConfigMessage

def test_ConfigMessage_init():
    configJSON = '{"key": "value"}'
    seq = 1
    total = 2
    featureID = '12345'
    message = ConfigMessage(configJSON, seq, total, featureID)
    
    assert message.messageType == MessageType.MT_CONFIG
    assert message.payload['mt'] == MessageType.MT_CONFIG
    assert message.payload['fi'] == featureID
    assert message.payload['se'] == seq
    assert message.payload['to'] == total
    assert message.payload['cs'] == configJSON