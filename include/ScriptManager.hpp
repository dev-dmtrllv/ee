#pragma once

#include "framework.hpp"
#include "SubSystem.hpp"
#include "Logger.hpp"
#include "BitMask.hpp"
#include "js_wrappers/JsClass.hpp"
#include "Types.hpp"
#include "Hash.hpp"

#define V8STR(str) v8::String::NewFromUtf8(isolate, str)

typedef const v8::FunctionCallbackInfo<v8::Value>& CallbackArgs;

namespace NovaEngine
{
	class ScriptManager;

	class ScriptManager : public SubSystem<>
	{
	public:
		struct RunInfo
		{
			RunInfo(ScriptManager* m, v8::Isolate* i, v8::Local<v8::Context>& c) : scriptManager(m), isolate(i), context(c) {}
			ScriptManager* scriptManager;
			v8::Isolate* isolate;
			v8::Local<v8::Context>& context;
		};

		typedef const char* Exception;

		template<typename Callback>
		static void iterateObjectKeys(v8::Local<v8::Object> obj, Callback callback)
		{
			using namespace v8;

			Local<Context> ctx = obj->CreationContext();
			Isolate* isolate = ctx->GetIsolate();

			Local<Array> keys = obj->GetOwnPropertyNames(ctx, PropertyFilter::ALL_PROPERTIES).ToLocalChecked();

			for (uint32_t i = 0, l = keys->Length(); i < l; i++)
			{
				Local<String> key = keys->Get(ctx, i).ToLocalChecked()->ToString(ctx).ToLocalChecked();
				String::Utf8Value keyVal(key);
				Local<Value> val = obj->Get(ctx, key).ToLocalChecked();
				callback(*keyVal, val);
			}
		}

		static inline void* getInternalFromArgs(V8CallbackArgs args, size_t field)
		{
			return args.This()->GetInternalField(field).As<v8::External>()->Value();
		}

		template<typename T>
		static inline T getInternalFromArgs(V8CallbackArgs args, size_t field)
		{
			return static_cast<T>(getInternalFromArgs(args, field));
		}

		static Engine* fetchEngineFromArgs(const v8::FunctionCallbackInfo<v8::Value>& args);
		static Engine* fetchEngineFromArgs(const v8::PropertyCallbackInfo<v8::Value>& args);
		static Engine* fetchEngineFromArgs(const v8::PropertyCallbackInfo<void>& args);
		static BitMask::Type getComponentHash(const v8::FunctionCallbackInfo<v8::Value>& args);

		static void printObject(v8::Isolate* isolate, const v8::Local<v8::Value>& o, const char* name = nullptr);
		static v8::Local<v8::String> objectToString(v8::Isolate* isolate, const v8::Local<v8::Value>& o);

	private:
		ENGINE_SUB_SYSTEM_CTOR(ScriptManager),
			createParams_(),
			isolate_(nullptr),
			context_(),
			scriptManagerReference_(),
			componentHashSymbol_(),
			modules_(),
			moduleRequireCounter_(0),
			registeredComponents_(),
			registeredClasses_()
		{}

		static void onRequire(const v8::FunctionCallbackInfo<v8::Value>& args);

		static std::unique_ptr<v8::Platform> platform_;

		v8::Isolate::CreateParams createParams_;
		v8::Isolate* isolate_;
		v8::Global<v8::Context> context_;
		v8::Global<v8::Number> scriptManagerReference_;
		v8::Global<v8::Private> componentHashSymbol_;
		std::unordered_map<std::string, v8::Global<v8::Object>> modules_;
		size_t moduleRequireCounter_;

		std::unordered_map<Hash, v8::Global<v8::FunctionTemplate>*> registeredComponents_;
		std::unordered_map<Hash, v8::Global<v8::FunctionTemplate>> registeredClasses_;

		std::string getRelativePath(const std::string& str);

		v8::Local<v8::Private> getComponentHashSymbol();
		
		template<typename Key, typename Val>
		inline void resetGlobalMap(std::unordered_map<Key, v8::Global<Val>>& map)
		{
			for (std::pair<const Key, v8::Global<Val>>& f : map)
				f.second.Reset();
		}

		inline void initializeGlobal(v8::Local<v8::Object> globalObj);

	protected:
		bool runScript(v8::Local<v8::Context> context, const char* scriptString);

		bool onInitialize();
		bool onTerminate();

	public:
		v8::Local<v8::String> createString(const char* string);
		v8::Local<v8::Function> createFunction(v8::FunctionCallback);

		v8::Isolate* isolate();
		v8::Local<v8::Context> context();

		void load(const char* path, bool isJsonModule = false);

		template<typename RunCallback>
		void run(RunCallback callback)
		{
			v8::Locker isolateLocker(isolate_);
			v8::Isolate::Scope isolate_scope(isolate_);

			v8::HandleScope handleScope(isolate_);
			v8::Local<v8::Context> context = v8::Local<v8::Context>::New(isolate_, context_);
			v8::Context::Scope contextScope(context);

			v8::TryCatch tryCatch = v8::TryCatch(isolate_);

			RunInfo runInfo(this, isolate_, context);

			callback(runInfo);

			if (tryCatch.HasCaught())
			{
				v8::Local<v8::String> exceptionStr = tryCatch.Exception()->ToString(isolate_);
				v8::String::Utf8Value val = v8::String::Utf8Value(isolate_, exceptionStr);
				Logger::get()->error(*val);
				throw Exception(*val);
			}
		}

		template<Types::DerivedFrom<JsWrappers::JsClass> JsComponentClass, typename NativeComponent>
		v8::Local<v8::Function> registerComponent(const char* name)
		{
			Hash nativeHash = Hasher::hash(typeid(NativeComponent).name());
			Hash hash = Hasher::hash(typeid(JsComponentClass).name());

			JsComponentClass c = JsComponentClass();

			v8::Local<v8::FunctionTemplate> f = c.create(engine(), isolate(), name);

			registeredClasses_[hash].Reset(isolate(), f);
			registeredComponents_[nativeHash] = &registeredClasses_[hash];
			
			
			const size_t count = (*f)->InstanceTemplate()->InternalFieldCount() + 2;
			(*f)->InstanceTemplate()->SetInternalFieldCount(count);

			return registeredClasses_[hash].Get(isolate())->GetFunction();
		}

		template<Types::DerivedFrom<JsWrappers::JsClass> JsClassType>
		v8::Local<v8::Function> registerClass(const char* name)
		{
			Hash hash = Hasher::hash(typeid(JsClassType).name());
			JsClassType c = JsClassType();
			v8::Local<v8::FunctionTemplate> f = c.create(engine(), isolate(), name);
			registeredClasses_[hash].Reset(isolate(), f);

			return registeredClasses_[hash].Get(isolate())->GetFunction();
		}

		template<Types::DerivedFrom<JsWrappers::JsClass> JsClassType>
		v8::Local<v8::FunctionTemplate> getClass()
		{
			Hash hash = Hasher::hash(typeid(JsClassType).name());
			assert(registeredClasses_.contains(hash));
			return registeredClasses_[hash].Get(isolate());
		}

		template<typename ComponentType>
		v8::Local<v8::FunctionTemplate> getComponentClass()
		{
			Hash hash = Hasher::hash(typeid(ComponentType).name());
			return getComponentClass(hash);
		}

		v8::Local<v8::FunctionTemplate> getComponentClass(Hash hash);

	private:
		void handleRequire(const v8::FunctionCallbackInfo<v8::Value>& args);
	};
}

#undef COMPONENT_BIT_MASK_VALUE
