// Code generated by protoc-gen-go. DO NOT EDIT.
// versions:
// 	protoc-gen-go v1.31.0
// 	protoc        v4.24.3
// source: common.proto

package scalellm

import (
	protoreflect "google.golang.org/protobuf/reflect/protoreflect"
	protoimpl "google.golang.org/protobuf/runtime/protoimpl"
	reflect "reflect"
	sync "sync"
)

const (
	// Verify that this generated code is sufficiently up-to-date.
	_ = protoimpl.EnforceVersion(20 - protoimpl.MinVersion)
	// Verify that runtime/protoimpl is sufficiently up-to-date.
	_ = protoimpl.EnforceVersion(protoimpl.MaxVersion - 20)
)

type Priority int32

const (
	Priority_DEFAULT Priority = 0
	Priority_HIGH    Priority = 1
	Priority_MEDIUM  Priority = 2
	Priority_LOW     Priority = 3
)

// Enum value maps for Priority.
var (
	Priority_name = map[int32]string{
		0: "DEFAULT",
		1: "HIGH",
		2: "MEDIUM",
		3: "LOW",
	}
	Priority_value = map[string]int32{
		"DEFAULT": 0,
		"HIGH":    1,
		"MEDIUM":  2,
		"LOW":     3,
	}
)

func (x Priority) Enum() *Priority {
	p := new(Priority)
	*p = x
	return p
}

func (x Priority) String() string {
	return protoimpl.X.EnumStringOf(x.Descriptor(), protoreflect.EnumNumber(x))
}

func (Priority) Descriptor() protoreflect.EnumDescriptor {
	return file_common_proto_enumTypes[0].Descriptor()
}

func (Priority) Type() protoreflect.EnumType {
	return &file_common_proto_enumTypes[0]
}

func (x Priority) Number() protoreflect.EnumNumber {
	return protoreflect.EnumNumber(x)
}

// Deprecated: Use Priority.Descriptor instead.
func (Priority) EnumDescriptor() ([]byte, []int) {
	return file_common_proto_rawDescGZIP(), []int{0}
}

type Usage struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	// the number of tokens in the prompt.
	PromptTokens *int32 `protobuf:"varint,1,opt,name=prompt_tokens,proto3,oneof" json:"prompt_tokens,omitempty"`
	// the number of tokens in the generated completion.
	CompletionTokens *int32 `protobuf:"varint,2,opt,name=completion_tokens,proto3,oneof" json:"completion_tokens,omitempty"`
	// the total number of tokens used in the request (prompt + completion).
	TotalTokens *int32 `protobuf:"varint,3,opt,name=total_tokens,proto3,oneof" json:"total_tokens,omitempty"`
}

func (x *Usage) Reset() {
	*x = Usage{}
	if protoimpl.UnsafeEnabled {
		mi := &file_common_proto_msgTypes[0]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *Usage) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*Usage) ProtoMessage() {}

func (x *Usage) ProtoReflect() protoreflect.Message {
	mi := &file_common_proto_msgTypes[0]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use Usage.ProtoReflect.Descriptor instead.
func (*Usage) Descriptor() ([]byte, []int) {
	return file_common_proto_rawDescGZIP(), []int{0}
}

func (x *Usage) GetPromptTokens() int32 {
	if x != nil && x.PromptTokens != nil {
		return *x.PromptTokens
	}
	return 0
}

func (x *Usage) GetCompletionTokens() int32 {
	if x != nil && x.CompletionTokens != nil {
		return *x.CompletionTokens
	}
	return 0
}

func (x *Usage) GetTotalTokens() int32 {
	if x != nil && x.TotalTokens != nil {
		return *x.TotalTokens
	}
	return 0
}

var File_common_proto protoreflect.FileDescriptor

var file_common_proto_rawDesc = []byte{
	0x0a, 0x0c, 0x63, 0x6f, 0x6d, 0x6d, 0x6f, 0x6e, 0x2e, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x12, 0x03,
	0x6c, 0x6c, 0x6d, 0x22, 0xc7, 0x01, 0x0a, 0x05, 0x55, 0x73, 0x61, 0x67, 0x65, 0x12, 0x29, 0x0a,
	0x0d, 0x70, 0x72, 0x6f, 0x6d, 0x70, 0x74, 0x5f, 0x74, 0x6f, 0x6b, 0x65, 0x6e, 0x73, 0x18, 0x01,
	0x20, 0x01, 0x28, 0x05, 0x48, 0x00, 0x52, 0x0d, 0x70, 0x72, 0x6f, 0x6d, 0x70, 0x74, 0x5f, 0x74,
	0x6f, 0x6b, 0x65, 0x6e, 0x73, 0x88, 0x01, 0x01, 0x12, 0x31, 0x0a, 0x11, 0x63, 0x6f, 0x6d, 0x70,
	0x6c, 0x65, 0x74, 0x69, 0x6f, 0x6e, 0x5f, 0x74, 0x6f, 0x6b, 0x65, 0x6e, 0x73, 0x18, 0x02, 0x20,
	0x01, 0x28, 0x05, 0x48, 0x01, 0x52, 0x11, 0x63, 0x6f, 0x6d, 0x70, 0x6c, 0x65, 0x74, 0x69, 0x6f,
	0x6e, 0x5f, 0x74, 0x6f, 0x6b, 0x65, 0x6e, 0x73, 0x88, 0x01, 0x01, 0x12, 0x27, 0x0a, 0x0c, 0x74,
	0x6f, 0x74, 0x61, 0x6c, 0x5f, 0x74, 0x6f, 0x6b, 0x65, 0x6e, 0x73, 0x18, 0x03, 0x20, 0x01, 0x28,
	0x05, 0x48, 0x02, 0x52, 0x0c, 0x74, 0x6f, 0x74, 0x61, 0x6c, 0x5f, 0x74, 0x6f, 0x6b, 0x65, 0x6e,
	0x73, 0x88, 0x01, 0x01, 0x42, 0x10, 0x0a, 0x0e, 0x5f, 0x70, 0x72, 0x6f, 0x6d, 0x70, 0x74, 0x5f,
	0x74, 0x6f, 0x6b, 0x65, 0x6e, 0x73, 0x42, 0x14, 0x0a, 0x12, 0x5f, 0x63, 0x6f, 0x6d, 0x70, 0x6c,
	0x65, 0x74, 0x69, 0x6f, 0x6e, 0x5f, 0x74, 0x6f, 0x6b, 0x65, 0x6e, 0x73, 0x42, 0x0f, 0x0a, 0x0d,
	0x5f, 0x74, 0x6f, 0x74, 0x61, 0x6c, 0x5f, 0x74, 0x6f, 0x6b, 0x65, 0x6e, 0x73, 0x2a, 0x36, 0x0a,
	0x08, 0x50, 0x72, 0x69, 0x6f, 0x72, 0x69, 0x74, 0x79, 0x12, 0x0b, 0x0a, 0x07, 0x44, 0x45, 0x46,
	0x41, 0x55, 0x4c, 0x54, 0x10, 0x00, 0x12, 0x08, 0x0a, 0x04, 0x48, 0x49, 0x47, 0x48, 0x10, 0x01,
	0x12, 0x0a, 0x0a, 0x06, 0x4d, 0x45, 0x44, 0x49, 0x55, 0x4d, 0x10, 0x02, 0x12, 0x07, 0x0a, 0x03,
	0x4c, 0x4f, 0x57, 0x10, 0x03, 0x42, 0x2a, 0x5a, 0x28, 0x67, 0x69, 0x74, 0x68, 0x75, 0x62, 0x2e,
	0x63, 0x6f, 0x6d, 0x2f, 0x76, 0x65, 0x63, 0x74, 0x6f, 0x72, 0x63, 0x68, 0x2d, 0x61, 0x69, 0x2f,
	0x73, 0x63, 0x61, 0x6c, 0x65, 0x6c, 0x6c, 0x6d, 0x3b, 0x73, 0x63, 0x61, 0x6c, 0x65, 0x6c, 0x6c,
	0x6d, 0x62, 0x06, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x33,
}

var (
	file_common_proto_rawDescOnce sync.Once
	file_common_proto_rawDescData = file_common_proto_rawDesc
)

func file_common_proto_rawDescGZIP() []byte {
	file_common_proto_rawDescOnce.Do(func() {
		file_common_proto_rawDescData = protoimpl.X.CompressGZIP(file_common_proto_rawDescData)
	})
	return file_common_proto_rawDescData
}

var file_common_proto_enumTypes = make([]protoimpl.EnumInfo, 1)
var file_common_proto_msgTypes = make([]protoimpl.MessageInfo, 1)
var file_common_proto_goTypes = []interface{}{
	(Priority)(0), // 0: llm.Priority
	(*Usage)(nil), // 1: llm.Usage
}
var file_common_proto_depIdxs = []int32{
	0, // [0:0] is the sub-list for method output_type
	0, // [0:0] is the sub-list for method input_type
	0, // [0:0] is the sub-list for extension type_name
	0, // [0:0] is the sub-list for extension extendee
	0, // [0:0] is the sub-list for field type_name
}

func init() { file_common_proto_init() }
func file_common_proto_init() {
	if File_common_proto != nil {
		return
	}
	if !protoimpl.UnsafeEnabled {
		file_common_proto_msgTypes[0].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*Usage); i {
			case 0:
				return &v.state
			case 1:
				return &v.sizeCache
			case 2:
				return &v.unknownFields
			default:
				return nil
			}
		}
	}
	file_common_proto_msgTypes[0].OneofWrappers = []interface{}{}
	type x struct{}
	out := protoimpl.TypeBuilder{
		File: protoimpl.DescBuilder{
			GoPackagePath: reflect.TypeOf(x{}).PkgPath(),
			RawDescriptor: file_common_proto_rawDesc,
			NumEnums:      1,
			NumMessages:   1,
			NumExtensions: 0,
			NumServices:   0,
		},
		GoTypes:           file_common_proto_goTypes,
		DependencyIndexes: file_common_proto_depIdxs,
		EnumInfos:         file_common_proto_enumTypes,
		MessageInfos:      file_common_proto_msgTypes,
	}.Build()
	File_common_proto = out.File
	file_common_proto_rawDesc = nil
	file_common_proto_goTypes = nil
	file_common_proto_depIdxs = nil
}
