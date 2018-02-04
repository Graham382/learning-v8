#include <iostream>
#include "gtest/gtest.h"
#include "v8.h"
#include "libplatform/libplatform.h"
#include "v8_test_fixture.h"
#include "src/factory.h"
#include "src/objects.h"
#include "src/objects-inl.h"
#include "src/api.h"

class ContextTest : public V8TestFixture {
};

TEST_F(ContextTest, Scopes) {
  const v8::Local<v8::ObjectTemplate> obt = v8::Local<v8::ObjectTemplate>();
  const v8::HandleScope handle_scope(V8TestFixture::isolate_);
  {
    v8::Handle<v8::Context> context1 = v8::Context::New(V8TestFixture::isolate_, nullptr, obt);
    v8::Context::Scope contextScope1(context1);
    // entered_contexts_ [context1], saved_contexts_[isolateContext]
    EXPECT_EQ(context1, V8TestFixture::isolate_->GetEnteredContext());
    EXPECT_EQ(context1, V8TestFixture::isolate_->GetCurrentContext());
    {
      v8::Handle<v8::Context> context2 = v8::Context::New(V8TestFixture::isolate_, nullptr, obt);
      v8::Context::Scope contextScope2(context2);
      // entered_contexts_ [context1, context2], saved_contexts[isolateContext, context1]
      EXPECT_EQ(context2, V8TestFixture::isolate_->GetEnteredContext());
      EXPECT_EQ(context2, V8TestFixture::isolate_->GetCurrentContext());
    }
  }
}

class MockExtension : public v8::Extension {
 public:
   MockExtension(const char* name) : v8::Extension(name) {}
};

TEST_F(ContextTest, ExtensionConfiguration) {
  const v8::HandleScope handle_scope(isolate_);
  const char* name = "mock";
  const char* names[] = {name};

  std::unique_ptr<MockExtension> mock_extension {new MockExtension{name}};
  v8::RegisterExtension(mock_extension.get());
  std::unique_ptr<v8::ExtensionConfiguration> ext{new v8::ExtensionConfiguration(1, names)};
  bool found = false;
  v8::Handle<v8::Context> context = v8::Context::New(isolate_,
                                         ext.get(),
                                         v8::Local<v8::ObjectTemplate>());

  for (v8::RegisteredExtension* it = v8::RegisteredExtension::first_extension(); it != nullptr; it = it->next()) {
    if (it->extension()->name() == name) {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(ContextTest, EmbedderData) {
  const v8::HandleScope handle_scope(isolate_);
  v8::Handle<v8::Context> context = v8::Context::New(isolate_,
                                         nullptr,
                                         v8::Local<v8::ObjectTemplate>());
  
  v8::Local<v8::String> str = v8::String::NewFromOneByte(isolate_, 
      reinterpret_cast<const uint8_t*>("embdata"),
      v8::NewStringType::kNormal,
      7).ToLocalChecked();
  context->SetEmbedderData(0, str);
  EXPECT_EQ(context->GetEmbedderData(0), str);

  v8::Handle<v8::Context> context2 = v8::Context::New(isolate_,
                                          nullptr,
                                          v8::Local<v8::ObjectTemplate>());
  // Note that the embedder data FixedArray is of length 3 initially and
  // and not zeroed out, wo we can't just check for null which I was my 
  // first thought. This is mainly to verify that context are indeed separate.
  EXPECT_NE(context2->GetEmbedderData(0), str);
}
