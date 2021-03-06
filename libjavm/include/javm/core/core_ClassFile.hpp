
#pragma once
#include <javm/javm_File.hpp>
#include <javm/core/core_FieldInfo.hpp>
#include <javm/core/core_ConstantPool.hpp>
#include <javm/core/core_ClassObject.hpp>
#include <cstdio>

#include <iostream>

namespace javm::core {

    class ClassFile;

    ValuePointerHolder HandleClassFileMethod(ClassFile *class_file, std::string name, std::string desc, Frame &frame);
    ValuePointerHolder HandleClassFileStaticFunction(ClassFile *class_file, std::string name, std::string desc, Frame &frame);

    class ClassFile : public ClassObject, public File {

        private:
            u32 magic;
            bool static_done;
            std::vector<CPInfo> pool;
            std::string class_name;
            std::string super_class_name;
            std::vector<std::string> interfaces;
            std::vector<FieldInfo> fields;
            std::vector<FieldInfo> methods;
            std::vector<AttributeInfo> attributes;

            void ProcessCPInfoArray(std::vector<CPInfo> &array) {
                for(auto &info: array) {
                    if(info.GetTag() == CPTag::Class) {
                        auto &data = info.GetClassData();
                        auto &name_data = this->pool[data.name_index - 1].GetUTF8Data();
                        data.processed_name = name_data.str;
                    }
                    else if(info.GetTag() == CPTag::NameAndType) {
                        auto &data = info.GetNameAndTypeData();
                        auto &name_data = this->pool[data.name_index - 1].GetUTF8Data();
                        auto &desc_data = this->pool[data.desc_index - 1].GetUTF8Data();
                        data.processed_name = name_data.str;
                        data.processed_desc = desc_data.str;
                    }
                    else if(info.GetTag() == CPTag::String) {
                        auto &data = info.GetStringData();
                        auto &string_data = this->pool[data.string_index - 1].GetUTF8Data();
                        data.processed_string = string_data.str;
                    }
                }
            }

            void ProcessFieldInfoArray(std::vector<FieldInfo> &array) {
                for(auto &info: array) {
                    auto &name_data = this->pool[info.GetNameIndex() - 1].GetUTF8Data();
                    auto &desc_data = this->pool[info.GetDescIndex() - 1].GetUTF8Data();
                    info.SetName(name_data.str);
                    info.SetDesc(desc_data.str);
                }
            }

            void ProcessAttributeInfoArray(std::vector<AttributeInfo> &array) {
                for(auto &info: array) {
                    auto &name_data = this->pool[info.GetNameIndex() - 1].GetUTF8Data();
                    info.SetName(name_data.str);
                }
            }

            void Load() {
                if(this->IsEmpty()) return;
                MemoryReader reader(this->GetFileData(), this->GetFileSize());
                this->magic = BE(reader.Read<u32>());
                u16 minor = BE(reader.Read<u16>());
                u16 major = BE(reader.Read<u16>());
                u16 const_count = BE(reader.Read<u16>());
                this->pool.reserve(const_count);

                u16 i = 1;
                while(i < const_count) {
                    auto &info_ref = this->pool.emplace_back(reader);
                    if(info_ref.GetTag() == CPTag::Double) {
                        this->pool.push_back(CPInfo());
                        i++;
                    }
                    i++;
                }

                this->ProcessCPInfoArray(this->pool);

                u16 access_flags = BE(reader.Read<u16>());
                u16 this_class_index = BE(reader.Read<u16>());
                auto &this_class_data = this->pool[this_class_index - 1].GetClassData();
                this->class_name = this_class_data.processed_name;
                u16 super_class_index = BE(reader.Read<u16>());
                auto &super_class_data = this->pool[super_class_index - 1].GetClassData();
                this->super_class_name = super_class_data.processed_name;

                u16 iface_count = BE(reader.Read<u16>());
                this->interfaces.reserve(iface_count);
                for(u16 i = 0; i < iface_count; i++) {
                    u16 iface_idx = BE(reader.Read<u16>());
                    this->interfaces.push_back(this->pool[iface_idx].GetClassData().processed_name);
                }

                u16 field_count = BE(reader.Read<u16>());
                this->fields.reserve(field_count);
                for(u16 i = 0; i < field_count; i++) {
                    auto &info_ref = this->fields.emplace_back(reader);
                    this->ProcessAttributeInfoArray(info_ref.GetAttributes());
                }

                u16 method_count = BE(reader.Read<u16>());
                this->methods.reserve(method_count);
                for(u16 i = 0; i < method_count; i++) {
                    auto &info_ref = this->methods.emplace_back(reader);
                    this->ProcessAttributeInfoArray(info_ref.GetAttributes());
                }

                u16 attr_count = BE(reader.Read<u16>());
                this->attributes.reserve(attr_count);
                for(u16 i = 0; i < attr_count; i++) {
                    this->attributes.emplace_back(reader);
                }

                this->ProcessFieldInfoArray(this->fields);
                this->ProcessFieldInfoArray(this->methods);
                this->ProcessAttributeInfoArray(this->attributes);
            }

        public:
            using File::File;

            ClassFile(std::string path) : File(path) {
                this->Load();
            }
            
            ClassFile(u8 *ptr, size_t ptr_sz, bool owns = false) : File(ptr, ptr_sz, owns) {
                this->Load();
            }

            ClassFile(ClassFile *other) : File(nullptr, 0), pool(other->GetConstantPool()), class_name(other->GetName()), super_class_name(other->GetSuperClassName()), fields(other->GetFields()), interfaces(other->GetInterfaces()), attributes(other->GetAttributes()), methods(other->GetMethods()) {
                this->Load();
            }

            bool IsEmpty() {
                return !this->IsValid();
            }

            std::vector<CPInfo> &GetConstantPool() override {
                return this->pool;
            }

            virtual ValuePointerHolder CreateInstance() override {
                return ValuePointerHolder::Create<ClassFile>(this);
            }

            virtual ValuePointerHolder GetField(std::string name) override {
                for(auto &field: this->fields) {
                    if(field.GetName() == name) {
                        return field.GetValue();
                    }
                }
                return ValuePointerHolder::CreateVoid();
            }

            virtual ValuePointerHolder GetStaticField(std::string name) override {
                for(auto &field: this->fields) {
                    if(field.GetName() == name) {
                        return field.GetValue();
                    }
                }
                return ValuePointerHolder::CreateVoid();
            }

            virtual void SetField(std::string name, ValuePointerHolder value) override {
                for(auto &field: this->fields) {
                    if(field.GetName() == name) {
                        field.SetValue(value);
                        break;
                    }
                }
            }

            virtual void SetStaticField(std::string name, ValuePointerHolder value) override {
                for(auto &field: this->fields) {
                    if(field.GetName() == name) {
                        field.SetValue(value);
                        break;
                    }
                }
            }

            virtual std::string GetName() override {
                return this->class_name;
            }

            virtual bool CanHandleMethod(std::string name, std::string desc) override {
                for(auto &method: this->methods) {
                    if(method.GetName() == name) {
                        if(method.GetDesc() == desc) {
                            return true;
                        }
                    }
                }
                return false;
            }

            virtual ValuePointerHolder HandleMethod(std::string name, std::string desc, Frame &frame) override {
                return HandleClassFileMethod(this, name, desc, frame);
            }

            virtual ValuePointerHolder HandleStaticFunction(std::string name, std::string desc, Frame &frame) override {
                if(name == JAVM_STATIC_BLOCK_METHOD_NAME) {
                    if(this->static_done) {
                        return ValuePointerHolder::CreateVoid();
                    }
                    else {
                        this->static_done = true;
                    }
                }
                return HandleClassFileStaticFunction(this, name, desc, frame);
            }

            std::string GetSuperClassName() {
                return this->super_class_name;
            }

            std::vector<FieldInfo> &GetMethods() {
                return this->methods;
            }

            std::vector<FieldInfo> &GetFields() {
                return this->fields;
            }

            std::vector<std::string> &GetInterfaces() {
                return this->interfaces;
            }

            std::vector<AttributeInfo> &GetAttributes() {
                return this->attributes;
            }
    };

}