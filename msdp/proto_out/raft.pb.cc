// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: raft.proto

#include "raft.pb.h"

#include <algorithm>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/wire_format_lite.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>

PROTOBUF_PRAGMA_INIT_SEG
constexpr LogEntryMessage::LogEntryMessage(
  ::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized)
  : m_command_(&::PROTOBUF_NAMESPACE_ID::internal::fixed_address_empty_string)
  , m_term_(0)
  , m_idx_(0){}
struct LogEntryMessageDefaultTypeInternal {
  constexpr LogEntryMessageDefaultTypeInternal()
    : _instance(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized{}) {}
  ~LogEntryMessageDefaultTypeInternal() {}
  union {
    LogEntryMessage _instance;
  };
};
PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT LogEntryMessageDefaultTypeInternal _LogEntryMessage_default_instance_;
constexpr LogsMessage::LogsMessage(
  ::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized)
  : entries_(){}
struct LogsMessageDefaultTypeInternal {
  constexpr LogsMessageDefaultTypeInternal()
    : _instance(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized{}) {}
  ~LogsMessageDefaultTypeInternal() {}
  union {
    LogsMessage _instance;
  };
};
PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT LogsMessageDefaultTypeInternal _LogsMessage_default_instance_;
static ::PROTOBUF_NAMESPACE_ID::Metadata file_level_metadata_raft_2eproto[2];
static constexpr ::PROTOBUF_NAMESPACE_ID::EnumDescriptor const** file_level_enum_descriptors_raft_2eproto = nullptr;
static constexpr ::PROTOBUF_NAMESPACE_ID::ServiceDescriptor const** file_level_service_descriptors_raft_2eproto = nullptr;

const uint32_t TableStruct_raft_2eproto::offsets[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  ~0u,  // no _has_bits_
  PROTOBUF_FIELD_OFFSET(::LogEntryMessage, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  ~0u,  // no _inlined_string_donated_
  PROTOBUF_FIELD_OFFSET(::LogEntryMessage, m_term_),
  PROTOBUF_FIELD_OFFSET(::LogEntryMessage, m_command_),
  PROTOBUF_FIELD_OFFSET(::LogEntryMessage, m_idx_),
  ~0u,  // no _has_bits_
  PROTOBUF_FIELD_OFFSET(::LogsMessage, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  ~0u,  // no _inlined_string_donated_
  PROTOBUF_FIELD_OFFSET(::LogsMessage, entries_),
};
static const ::PROTOBUF_NAMESPACE_ID::internal::MigrationSchema schemas[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  { 0, -1, -1, sizeof(::LogEntryMessage)},
  { 9, -1, -1, sizeof(::LogsMessage)},
};

static ::PROTOBUF_NAMESPACE_ID::Message const * const file_default_instances[] = {
  reinterpret_cast<const ::PROTOBUF_NAMESPACE_ID::Message*>(&::_LogEntryMessage_default_instance_),
  reinterpret_cast<const ::PROTOBUF_NAMESPACE_ID::Message*>(&::_LogsMessage_default_instance_),
};

const char descriptor_table_protodef_raft_2eproto[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) =
  "\n\nraft.proto\"C\n\017LogEntryMessage\022\016\n\006m_ter"
  "m\030\001 \001(\005\022\021\n\tm_command\030\002 \001(\t\022\r\n\005m_idx\030\003 \001("
  "\005\"0\n\013LogsMessage\022!\n\007entries\030\001 \003(\0132\020.LogE"
  "ntryMessageB\003\200\001\001b\006proto3"
  ;
static ::PROTOBUF_NAMESPACE_ID::internal::once_flag descriptor_table_raft_2eproto_once;
const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable descriptor_table_raft_2eproto = {
  false, false, 144, descriptor_table_protodef_raft_2eproto, "raft.proto", 
  &descriptor_table_raft_2eproto_once, nullptr, 0, 2,
  schemas, file_default_instances, TableStruct_raft_2eproto::offsets,
  file_level_metadata_raft_2eproto, file_level_enum_descriptors_raft_2eproto, file_level_service_descriptors_raft_2eproto,
};
PROTOBUF_ATTRIBUTE_WEAK const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable* descriptor_table_raft_2eproto_getter() {
  return &descriptor_table_raft_2eproto;
}

// Force running AddDescriptors() at dynamic initialization time.
PROTOBUF_ATTRIBUTE_INIT_PRIORITY static ::PROTOBUF_NAMESPACE_ID::internal::AddDescriptorsRunner dynamic_init_dummy_raft_2eproto(&descriptor_table_raft_2eproto);

// ===================================================================

class LogEntryMessage::_Internal {
 public:
};

LogEntryMessage::LogEntryMessage(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                         bool is_message_owned)
  : ::PROTOBUF_NAMESPACE_ID::Message(arena, is_message_owned) {
  SharedCtor();
  if (!is_message_owned) {
    RegisterArenaDtor(arena);
  }
  // @@protoc_insertion_point(arena_constructor:LogEntryMessage)
}
LogEntryMessage::LogEntryMessage(const LogEntryMessage& from)
  : ::PROTOBUF_NAMESPACE_ID::Message() {
  _internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
  m_command_.UnsafeSetDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
  #ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
    m_command_.Set(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), "", GetArenaForAllocation());
  #endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (!from._internal_m_command().empty()) {
    m_command_.Set(::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{}, from._internal_m_command(), 
      GetArenaForAllocation());
  }
  ::memcpy(&m_term_, &from.m_term_,
    static_cast<size_t>(reinterpret_cast<char*>(&m_idx_) -
    reinterpret_cast<char*>(&m_term_)) + sizeof(m_idx_));
  // @@protoc_insertion_point(copy_constructor:LogEntryMessage)
}

inline void LogEntryMessage::SharedCtor() {
m_command_.UnsafeSetDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
  m_command_.Set(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), "", GetArenaForAllocation());
#endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
::memset(reinterpret_cast<char*>(this) + static_cast<size_t>(
    reinterpret_cast<char*>(&m_term_) - reinterpret_cast<char*>(this)),
    0, static_cast<size_t>(reinterpret_cast<char*>(&m_idx_) -
    reinterpret_cast<char*>(&m_term_)) + sizeof(m_idx_));
}

LogEntryMessage::~LogEntryMessage() {
  // @@protoc_insertion_point(destructor:LogEntryMessage)
  if (GetArenaForAllocation() != nullptr) return;
  SharedDtor();
  _internal_metadata_.Delete<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
}

inline void LogEntryMessage::SharedDtor() {
  GOOGLE_DCHECK(GetArenaForAllocation() == nullptr);
  m_command_.DestroyNoArena(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
}

void LogEntryMessage::ArenaDtor(void* object) {
  LogEntryMessage* _this = reinterpret_cast< LogEntryMessage* >(object);
  (void)_this;
}
void LogEntryMessage::RegisterArenaDtor(::PROTOBUF_NAMESPACE_ID::Arena*) {
}
void LogEntryMessage::SetCachedSize(int size) const {
  _cached_size_.Set(size);
}

void LogEntryMessage::Clear() {
// @@protoc_insertion_point(message_clear_start:LogEntryMessage)
  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  m_command_.ClearToEmpty();
  ::memset(&m_term_, 0, static_cast<size_t>(
      reinterpret_cast<char*>(&m_idx_) -
      reinterpret_cast<char*>(&m_term_)) + sizeof(m_idx_));
  _internal_metadata_.Clear<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
}

const char* LogEntryMessage::_InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) {
#define CHK_(x) if (PROTOBUF_PREDICT_FALSE(!(x))) goto failure
  while (!ctx->Done(&ptr)) {
    uint32_t tag;
    ptr = ::PROTOBUF_NAMESPACE_ID::internal::ReadTag(ptr, &tag);
    switch (tag >> 3) {
      // int32 m_term = 1;
      case 1:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 8)) {
          m_term_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint32(&ptr);
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      // string m_command = 2;
      case 2:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 18)) {
          auto str = _internal_mutable_m_command();
          ptr = ::PROTOBUF_NAMESPACE_ID::internal::InlineGreedyStringParser(str, ptr, ctx);
          CHK_(::PROTOBUF_NAMESPACE_ID::internal::VerifyUTF8(str, "LogEntryMessage.m_command"));
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      // int32 m_idx = 3;
      case 3:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 24)) {
          m_idx_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint32(&ptr);
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      default:
        goto handle_unusual;
    }  // switch
  handle_unusual:
    if ((tag == 0) || ((tag & 7) == 4)) {
      CHK_(ptr);
      ctx->SetLastTag(tag);
      goto message_done;
    }
    ptr = UnknownFieldParse(
        tag,
        _internal_metadata_.mutable_unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(),
        ptr, ctx);
    CHK_(ptr != nullptr);
  }  // while
message_done:
  return ptr;
failure:
  ptr = nullptr;
  goto message_done;
#undef CHK_
}

uint8_t* LogEntryMessage::_InternalSerialize(
    uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const {
  // @@protoc_insertion_point(serialize_to_array_start:LogEntryMessage)
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  // int32 m_term = 1;
  if (this->_internal_m_term() != 0) {
    target = stream->EnsureSpace(target);
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::WriteInt32ToArray(1, this->_internal_m_term(), target);
  }

  // string m_command = 2;
  if (!this->_internal_m_command().empty()) {
    ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::VerifyUtf8String(
      this->_internal_m_command().data(), static_cast<int>(this->_internal_m_command().length()),
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::SERIALIZE,
      "LogEntryMessage.m_command");
    target = stream->WriteStringMaybeAliased(
        2, this->_internal_m_command(), target);
  }

  // int32 m_idx = 3;
  if (this->_internal_m_idx() != 0) {
    target = stream->EnsureSpace(target);
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::WriteInt32ToArray(3, this->_internal_m_idx(), target);
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormat::InternalSerializeUnknownFieldsToArray(
        _internal_metadata_.unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(::PROTOBUF_NAMESPACE_ID::UnknownFieldSet::default_instance), target, stream);
  }
  // @@protoc_insertion_point(serialize_to_array_end:LogEntryMessage)
  return target;
}

size_t LogEntryMessage::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:LogEntryMessage)
  size_t total_size = 0;

  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  // string m_command = 2;
  if (!this->_internal_m_command().empty()) {
    total_size += 1 +
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::StringSize(
        this->_internal_m_command());
  }

  // int32 m_term = 1;
  if (this->_internal_m_term() != 0) {
    total_size += ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::Int32SizePlusOne(this->_internal_m_term());
  }

  // int32 m_idx = 3;
  if (this->_internal_m_idx() != 0) {
    total_size += ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::Int32SizePlusOne(this->_internal_m_idx());
  }

  return MaybeComputeUnknownFieldsSize(total_size, &_cached_size_);
}

const ::PROTOBUF_NAMESPACE_ID::Message::ClassData LogEntryMessage::_class_data_ = {
    ::PROTOBUF_NAMESPACE_ID::Message::CopyWithSizeCheck,
    LogEntryMessage::MergeImpl
};
const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*LogEntryMessage::GetClassData() const { return &_class_data_; }

void LogEntryMessage::MergeImpl(::PROTOBUF_NAMESPACE_ID::Message* to,
                      const ::PROTOBUF_NAMESPACE_ID::Message& from) {
  static_cast<LogEntryMessage *>(to)->MergeFrom(
      static_cast<const LogEntryMessage &>(from));
}


void LogEntryMessage::MergeFrom(const LogEntryMessage& from) {
// @@protoc_insertion_point(class_specific_merge_from_start:LogEntryMessage)
  GOOGLE_DCHECK_NE(&from, this);
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  if (!from._internal_m_command().empty()) {
    _internal_set_m_command(from._internal_m_command());
  }
  if (from._internal_m_term() != 0) {
    _internal_set_m_term(from._internal_m_term());
  }
  if (from._internal_m_idx() != 0) {
    _internal_set_m_idx(from._internal_m_idx());
  }
  _internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
}

void LogEntryMessage::CopyFrom(const LogEntryMessage& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:LogEntryMessage)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool LogEntryMessage::IsInitialized() const {
  return true;
}

void LogEntryMessage::InternalSwap(LogEntryMessage* other) {
  using std::swap;
  auto* lhs_arena = GetArenaForAllocation();
  auto* rhs_arena = other->GetArenaForAllocation();
  _internal_metadata_.InternalSwap(&other->_internal_metadata_);
  ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::InternalSwap(
      &::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(),
      &m_command_, lhs_arena,
      &other->m_command_, rhs_arena
  );
  ::PROTOBUF_NAMESPACE_ID::internal::memswap<
      PROTOBUF_FIELD_OFFSET(LogEntryMessage, m_idx_)
      + sizeof(LogEntryMessage::m_idx_)
      - PROTOBUF_FIELD_OFFSET(LogEntryMessage, m_term_)>(
          reinterpret_cast<char*>(&m_term_),
          reinterpret_cast<char*>(&other->m_term_));
}

::PROTOBUF_NAMESPACE_ID::Metadata LogEntryMessage::GetMetadata() const {
  return ::PROTOBUF_NAMESPACE_ID::internal::AssignDescriptors(
      &descriptor_table_raft_2eproto_getter, &descriptor_table_raft_2eproto_once,
      file_level_metadata_raft_2eproto[0]);
}

// ===================================================================

class LogsMessage::_Internal {
 public:
};

LogsMessage::LogsMessage(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                         bool is_message_owned)
  : ::PROTOBUF_NAMESPACE_ID::Message(arena, is_message_owned),
  entries_(arena) {
  SharedCtor();
  if (!is_message_owned) {
    RegisterArenaDtor(arena);
  }
  // @@protoc_insertion_point(arena_constructor:LogsMessage)
}
LogsMessage::LogsMessage(const LogsMessage& from)
  : ::PROTOBUF_NAMESPACE_ID::Message(),
      entries_(from.entries_) {
  _internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
  // @@protoc_insertion_point(copy_constructor:LogsMessage)
}

inline void LogsMessage::SharedCtor() {
}

LogsMessage::~LogsMessage() {
  // @@protoc_insertion_point(destructor:LogsMessage)
  if (GetArenaForAllocation() != nullptr) return;
  SharedDtor();
  _internal_metadata_.Delete<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
}

inline void LogsMessage::SharedDtor() {
  GOOGLE_DCHECK(GetArenaForAllocation() == nullptr);
}

void LogsMessage::ArenaDtor(void* object) {
  LogsMessage* _this = reinterpret_cast< LogsMessage* >(object);
  (void)_this;
}
void LogsMessage::RegisterArenaDtor(::PROTOBUF_NAMESPACE_ID::Arena*) {
}
void LogsMessage::SetCachedSize(int size) const {
  _cached_size_.Set(size);
}

void LogsMessage::Clear() {
// @@protoc_insertion_point(message_clear_start:LogsMessage)
  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  entries_.Clear();
  _internal_metadata_.Clear<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
}

const char* LogsMessage::_InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) {
#define CHK_(x) if (PROTOBUF_PREDICT_FALSE(!(x))) goto failure
  while (!ctx->Done(&ptr)) {
    uint32_t tag;
    ptr = ::PROTOBUF_NAMESPACE_ID::internal::ReadTag(ptr, &tag);
    switch (tag >> 3) {
      // repeated .LogEntryMessage entries = 1;
      case 1:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 10)) {
          ptr -= 1;
          do {
            ptr += 1;
            ptr = ctx->ParseMessage(_internal_add_entries(), ptr);
            CHK_(ptr);
            if (!ctx->DataAvailable(ptr)) break;
          } while (::PROTOBUF_NAMESPACE_ID::internal::ExpectTag<10>(ptr));
        } else
          goto handle_unusual;
        continue;
      default:
        goto handle_unusual;
    }  // switch
  handle_unusual:
    if ((tag == 0) || ((tag & 7) == 4)) {
      CHK_(ptr);
      ctx->SetLastTag(tag);
      goto message_done;
    }
    ptr = UnknownFieldParse(
        tag,
        _internal_metadata_.mutable_unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(),
        ptr, ctx);
    CHK_(ptr != nullptr);
  }  // while
message_done:
  return ptr;
failure:
  ptr = nullptr;
  goto message_done;
#undef CHK_
}

uint8_t* LogsMessage::_InternalSerialize(
    uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const {
  // @@protoc_insertion_point(serialize_to_array_start:LogsMessage)
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  // repeated .LogEntryMessage entries = 1;
  for (unsigned int i = 0,
      n = static_cast<unsigned int>(this->_internal_entries_size()); i < n; i++) {
    target = stream->EnsureSpace(target);
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::
      InternalWriteMessage(1, this->_internal_entries(i), target, stream);
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormat::InternalSerializeUnknownFieldsToArray(
        _internal_metadata_.unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(::PROTOBUF_NAMESPACE_ID::UnknownFieldSet::default_instance), target, stream);
  }
  // @@protoc_insertion_point(serialize_to_array_end:LogsMessage)
  return target;
}

size_t LogsMessage::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:LogsMessage)
  size_t total_size = 0;

  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  // repeated .LogEntryMessage entries = 1;
  total_size += 1UL * this->_internal_entries_size();
  for (const auto& msg : this->entries_) {
    total_size +=
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::MessageSize(msg);
  }

  return MaybeComputeUnknownFieldsSize(total_size, &_cached_size_);
}

const ::PROTOBUF_NAMESPACE_ID::Message::ClassData LogsMessage::_class_data_ = {
    ::PROTOBUF_NAMESPACE_ID::Message::CopyWithSizeCheck,
    LogsMessage::MergeImpl
};
const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*LogsMessage::GetClassData() const { return &_class_data_; }

void LogsMessage::MergeImpl(::PROTOBUF_NAMESPACE_ID::Message* to,
                      const ::PROTOBUF_NAMESPACE_ID::Message& from) {
  static_cast<LogsMessage *>(to)->MergeFrom(
      static_cast<const LogsMessage &>(from));
}


void LogsMessage::MergeFrom(const LogsMessage& from) {
// @@protoc_insertion_point(class_specific_merge_from_start:LogsMessage)
  GOOGLE_DCHECK_NE(&from, this);
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  entries_.MergeFrom(from.entries_);
  _internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
}

void LogsMessage::CopyFrom(const LogsMessage& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:LogsMessage)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool LogsMessage::IsInitialized() const {
  return true;
}

void LogsMessage::InternalSwap(LogsMessage* other) {
  using std::swap;
  _internal_metadata_.InternalSwap(&other->_internal_metadata_);
  entries_.InternalSwap(&other->entries_);
}

::PROTOBUF_NAMESPACE_ID::Metadata LogsMessage::GetMetadata() const {
  return ::PROTOBUF_NAMESPACE_ID::internal::AssignDescriptors(
      &descriptor_table_raft_2eproto_getter, &descriptor_table_raft_2eproto_once,
      file_level_metadata_raft_2eproto[1]);
}

// @@protoc_insertion_point(namespace_scope)
PROTOBUF_NAMESPACE_OPEN
template<> PROTOBUF_NOINLINE ::LogEntryMessage* Arena::CreateMaybeMessage< ::LogEntryMessage >(Arena* arena) {
  return Arena::CreateMessageInternal< ::LogEntryMessage >(arena);
}
template<> PROTOBUF_NOINLINE ::LogsMessage* Arena::CreateMaybeMessage< ::LogsMessage >(Arena* arena) {
  return Arena::CreateMessageInternal< ::LogsMessage >(arena);
}
PROTOBUF_NAMESPACE_CLOSE

// @@protoc_insertion_point(global_scope)
#include <google/protobuf/port_undef.inc>
