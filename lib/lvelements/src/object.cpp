#include "object.h"
#include "live/elements/engine.h"
#include "live/elements/element.h"
#include "element_p.h"
#include "context_p.h"
#include "value_p.h"
#include "v8.h"

namespace lv{ namespace el{


// Object implementation
// ----------------------------------------------------------------------

class ObjectPrivate{

public:
    ObjectPrivate(Engine* pengine, const v8::Local<v8::Object>& val)
        : engine(pengine)
    {
        data.Reset(engine->isolate(), val);
    }

public:
    Engine*                    engine;
    v8::Persistent<v8::Object> data;
    int*                       ref;
};


Object::Object(Engine* engine)
    : m_d(new ObjectPrivate(engine, v8::Local<v8::Object>()))
    , m_ref(new int)
{
    m_d->ref = m_ref;
    ++(*m_ref);
}

Object::Object(Engine* engine, const v8::Local<v8::Object> &data)
    : m_d(nullptr)
    , m_ref(nullptr)
{
    v8::Local<v8::Value> shared = data->Get(v8::String::NewFromUtf8(engine->isolate(), "__shared__"));

    if ( shared->IsExternal() ){
        m_d = reinterpret_cast<ObjectPrivate*>(v8::Local<v8::External>::Cast(shared)->Value());
        m_ref = m_d->ref;
    } else {
        m_d = new ObjectPrivate(engine, data);
        m_ref = new int;
        m_d->ref = m_ref;
        data->Set(v8::String::NewFromUtf8(engine->isolate(), "__shared__"), v8::External::New(engine->isolate(), m_d));
    }

    ++(*m_ref);
}

Object::~Object(){
    --(*m_ref);
    if ( *m_ref == 0 ){
        if ( !m_d->data.IsEmpty() ){
            v8::Local<v8::Object> v = m_d->data.Get(m_d->engine->isolate());
            v->Set(v8::String::NewFromUtf8(m_d->engine->isolate(), "__shared__"), v8::Undefined(m_d->engine->isolate()));
            m_d->data.SetWeak();
        }

        delete m_ref;
        delete m_d;
    }
}

Object Object::create(Engine *engine){
    return Object(engine, v8::Object::New(engine->isolate()));
}

std::string Object::toString() const{
    if ( m_d->data.IsEmpty() )
        return std::string();

    v8::Local<v8::Object> s = m_d->data.Get(m_d->engine->isolate());
    return *v8::String::Utf8Value(s->ToString(m_d->engine->isolate()));
}

Buffer Object::toBuffer() const{
    if ( m_d->data.IsEmpty() )
        return Buffer(nullptr, 0);

    v8::Local<v8::Object> b = m_d->data.Get(m_d->engine->isolate());
    return Buffer(v8::Local<v8::ArrayBuffer>::Cast(b));
}

Object Object::create(ObjectPrivate *d, int *ref){
    return Object(d, ref);
}

Object::Object(ObjectPrivate *d, int *ref)
    : m_d(d)
    , m_ref(ref){
    ++(*m_ref);
}

Object::Object(const Object &other)
    : m_d(other.m_d)
    , m_ref(other.m_ref)
{
    ++(*m_ref);
}

Object &Object::operator =(const Object &other){
    if ( this != &other ){
        --(*m_ref);
        if ( *m_ref == 0 ){
            delete m_d;
            delete m_ref;
        }
        m_d = other.m_d;
        m_ref = other.m_ref;
        ++(*m_ref);
    }
    return *this;
}

bool Object::operator ==(const Object &other){
    return ( m_d == other.m_d && m_ref == other.m_ref );
}

bool Object::isNull() const{
    if ( m_d->data.IsEmpty() )
        return true;
    return m_d->data.Get(m_d->engine->isolate())->IsNullOrUndefined();
}

bool Object::isString() const{
    v8::Local<v8::Object> lo = m_d->data.Get(m_d->engine->isolate());
    return lo->IsString() || lo->IsStringObject();
}

bool Object::isPoint() const{
    v8::Local<v8::Object> lo = m_d->data.Get(m_d->engine->isolate());
    v8::Local<v8::FunctionTemplate> lotpl = m_d->engine->pointTemplate();
    return lotpl->HasInstance(lo);
}

void Object::toPoint(double &x, double &y) const{
    v8::Local<v8::Object> lo = m_d->data.Get(m_d->engine->isolate());
    if ( !isPoint() ){
        lv::Exception e = CREATE_EXCEPTION(lv::Exception, "Object is not of Point type.", 1);
        m_d->engine->throwError(&e, nullptr);
        return;
    }

    x = lo->Get(v8::String::NewFromUtf8(m_d->engine->isolate(), "x", v8::String::kInternalizedString))->NumberValue();
    y = lo->Get(v8::String::NewFromUtf8(m_d->engine->isolate(), "y", v8::String::kInternalizedString))->NumberValue();
}

bool Object::isSize() const{
    v8::Local<v8::Object> lo = m_d->data.Get(m_d->engine->isolate());
    v8::Local<v8::FunctionTemplate> lotpl = m_d->engine->sizeTemplate();
    return lotpl->HasInstance(lo);
}

void Object::toSize(double &width, double &height) const{
    v8::Local<v8::Object> lo = m_d->data.Get(m_d->engine->isolate());
    if ( !isSize() ){
        lv::Exception e = CREATE_EXCEPTION(lv::Exception, "Object is not of Size type.", 1);
        m_d->engine->throwError(&e, nullptr);
        return;
    }

    width =  lo->Get(v8::String::NewFromUtf8(m_d->engine->isolate(), "width", v8::String::kInternalizedString))->NumberValue();
    height = lo->Get(v8::String::NewFromUtf8(m_d->engine->isolate(), "height", v8::String::kInternalizedString))->NumberValue();
}

bool Object::isRectangle() const{
    v8::Local<v8::Object> lo = m_d->data.Get(m_d->engine->isolate());
    v8::Local<v8::FunctionTemplate> lotpl = m_d->engine->rectangleTemplate();
    return lotpl->HasInstance(lo);
}

void Object::toRectangle(double &x, double &y, double &width, double &height){
    v8::Local<v8::Object> lo = m_d->data.Get(m_d->engine->isolate());
    if ( !isRectangle() ){
        lv::Exception e = CREATE_EXCEPTION(lv::Exception, "Object is not of Rectangle type.", 1);
        m_d->engine->throwError(&e, nullptr);
        return;
    }

    x = lo->Get(v8::String::NewFromUtf8(m_d->engine->isolate(), "x", v8::String::kInternalizedString))->NumberValue();
    y = lo->Get(v8::String::NewFromUtf8(m_d->engine->isolate(), "y", v8::String::kInternalizedString))->NumberValue();
    width = lo->Get(v8::String::NewFromUtf8(m_d->engine->isolate(), "width", v8::String::kInternalizedString))->NumberValue();
    height = lo->Get(v8::String::NewFromUtf8(m_d->engine->isolate(), "height", v8::String::kInternalizedString))->NumberValue();
}

bool Object::isDate() const{
    v8::Local<v8::Object> lo = m_d->data.Get(m_d->engine->isolate());
    return lo->IsDate();
}

double Object::toDate() const{
    v8::Local<v8::Object> lo = m_d->data.Get(m_d->engine->isolate());
    return v8::Local<v8::Date>::Cast(lo)->ValueOf();
}

v8::Local<v8::Object> Object::data() const{
    return m_d->data.Get(m_d->engine->isolate());
}



// LocalObject implementation
// ----------------------------------------------------------------------

class LocalObjectPrivate{
public:
    LocalObjectPrivate(const v8::Local<v8::Object>& o) : data(o){}

    v8::Local<v8::Object> data;
};


LocalObject::LocalObject(Object o)
    : m_d(new LocalObjectPrivate(o.data()))
{
}

LocalObject::LocalObject(Context *context)
    : m_d(new LocalObjectPrivate(context->asLocal()->Global()))
{
}

LocalObject::LocalObject(const v8::Local<v8::Object> &vo)
    : m_d(new LocalObjectPrivate(vo))
{
}

LocalObject::~LocalObject(){
    delete m_d;
}

LocalValue LocalObject::get(int index){
    return LocalValue(m_d->data->Get(index));
}

LocalValue LocalObject::get(const LocalValue &key){
    return m_d->data->Get(key.data());
}

LocalValue LocalObject::get(Engine *engine, const std::string &str){
    return get(LocalValue(engine, str));
}

void LocalObject::set(int index, const LocalValue &value){
    m_d->data->Set(index, value.data());
}

void LocalObject::set(const LocalValue &key, const LocalValue &value){
    m_d->data->Set(key.data(), value.data());
}

void LocalObject::set(Engine *engine, const std::string &key, const LocalValue &value){
    set(LocalValue(engine, key), value);
}


}}// namespace lv, el
