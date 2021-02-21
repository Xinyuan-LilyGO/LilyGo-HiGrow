# -*- coding: utf-8 -*-
# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: measurements.proto
"""Generated protocol buffer code."""
from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from google.protobuf import reflection as _reflection
from google.protobuf import symbol_database as _symbol_database
# @@protoc_insertion_point(imports)

_sym_db = _symbol_database.Default()




DESCRIPTOR = _descriptor.FileDescriptor(
  name='measurements.proto',
  package='ttgo.proto',
  syntax='proto3',
  serialized_options=None,
  create_key=_descriptor._internal_create_key,
  serialized_pb=b'\n\x12measurements.proto\x12\nttgo.proto\"\x87\x02\n\x0cMeasurements\x12\x12\n\nerror_code\x18\x01 \x01(\r\x12\x0b\n\x03lux\x18\x02 \x01(\x02\x12\x10\n\x08humidity\x18\x03 \x01(\x02\x12\x15\n\rtemperature_C\x18\x04 \x01(\x02\x12\x0c\n\x04soil\x18\x05 \x01(\x02\x12\x0c\n\x04salt\x18\x06 \x01(\x02\x12\x12\n\nbattery_mV\x18\x07 \x01(\x02\x12\x11\n\ttimestamp\x18\x08 \x01(\r\x12\x18\n\x10\x66w_version_major\x18\t \x01(\r\x12\x18\n\x10\x66w_version_minor\x18\n \x01(\r\x12\x18\n\x10\x66w_version_patch\x18\x0b \x01(\r\x12\x1c\n\x14num_dht_failed_reads\x18\x0c \x01(\rb\x06proto3'
)




_MEASUREMENTS = _descriptor.Descriptor(
  name='Measurements',
  full_name='ttgo.proto.Measurements',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='error_code', full_name='ttgo.proto.Measurements.error_code', index=0,
      number=1, type=13, cpp_type=3, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='lux', full_name='ttgo.proto.Measurements.lux', index=1,
      number=2, type=2, cpp_type=6, label=1,
      has_default_value=False, default_value=float(0),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='humidity', full_name='ttgo.proto.Measurements.humidity', index=2,
      number=3, type=2, cpp_type=6, label=1,
      has_default_value=False, default_value=float(0),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='temperature_C', full_name='ttgo.proto.Measurements.temperature_C', index=3,
      number=4, type=2, cpp_type=6, label=1,
      has_default_value=False, default_value=float(0),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='soil', full_name='ttgo.proto.Measurements.soil', index=4,
      number=5, type=2, cpp_type=6, label=1,
      has_default_value=False, default_value=float(0),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='salt', full_name='ttgo.proto.Measurements.salt', index=5,
      number=6, type=2, cpp_type=6, label=1,
      has_default_value=False, default_value=float(0),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='battery_mV', full_name='ttgo.proto.Measurements.battery_mV', index=6,
      number=7, type=2, cpp_type=6, label=1,
      has_default_value=False, default_value=float(0),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='timestamp', full_name='ttgo.proto.Measurements.timestamp', index=7,
      number=8, type=13, cpp_type=3, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='fw_version_major', full_name='ttgo.proto.Measurements.fw_version_major', index=8,
      number=9, type=13, cpp_type=3, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='fw_version_minor', full_name='ttgo.proto.Measurements.fw_version_minor', index=9,
      number=10, type=13, cpp_type=3, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='fw_version_patch', full_name='ttgo.proto.Measurements.fw_version_patch', index=10,
      number=11, type=13, cpp_type=3, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='num_dht_failed_reads', full_name='ttgo.proto.Measurements.num_dht_failed_reads', index=11,
      number=12, type=13, cpp_type=3, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=35,
  serialized_end=298,
)

DESCRIPTOR.message_types_by_name['Measurements'] = _MEASUREMENTS
_sym_db.RegisterFileDescriptor(DESCRIPTOR)

Measurements = _reflection.GeneratedProtocolMessageType('Measurements', (_message.Message,), {
  'DESCRIPTOR' : _MEASUREMENTS,
  '__module__' : 'measurements_pb2'
  # @@protoc_insertion_point(class_scope:ttgo.proto.Measurements)
  })
_sym_db.RegisterMessage(Measurements)


# @@protoc_insertion_point(module_scope)