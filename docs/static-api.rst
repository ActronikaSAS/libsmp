============
 Static API
============

To support the embedded platform on which dynamic allocation is not suitable, we
provide a set of *static* APIs used to construct object using static storage and
a some macros to define the boilerplate code. The code below show usages of
these macros:

.. code-block:: c

   #define SMP_ENABLE_STATIC_API
   #include <libsmp.h>

   static void on_new_message(SmpContext *ctx, SmpMessage *msg, void *userdata)
   {
   }

   static void on_error(SmpContext *ctx, SmpError error, void *userdata)
   {
   }

   static SmpEventCallbacks cbs = {
       .new_message_cb = on_new_message,
       .error_cb = on_error,
   };

   SMP_DEFINE_STATIC_CONTEXT(my_smp_context, 128, 128, 128, 16);

   SMP_DEFINE_STATIC_MESSAGE(my_message, 16);

   int main()
   {
       SmpContext *ctx;
       SmpMessage *msg;int msgid = 42;

       ctx = my_smp_context_create(&cbs, NULL);
       if (ctx == NULL)
           return 1;

       msg = my_message_create(msgid);
       if (msg == NULL)
           return 1;

       smp_context_send_message(ctx, msg);return 0;
   }
