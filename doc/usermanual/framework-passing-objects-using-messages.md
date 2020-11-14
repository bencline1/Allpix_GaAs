Passing Objects using Messages
------------------------------

Communication between modules is performed by the exchange of messages.
Messages are templated instantiations of the `Message` class carrying a
vector of objects. The list of objects available in the Allpix² objects
library are discussed in Chapter [Objects](objects.md). The messaging system has
a dispatching mechanism to send messages and a receiving part that
fetches incoming messages. Messages are always received by modules in
the order they have been dispatched by preceding modules.

The dispatching module can specify an optional name for the messages,
but modules should normally not specify this name directly. If the name
is not given (or equal to `-`) the `output` parameter of the module is
used to determine the name of the message, defaulting to an empty
string. Dispatching messages to their receivers is then performed
following these rules:

1.  The receiving module will receive a message if it has the exact same
    type as the message dispatched (thus carrying the same objects). If
    the receiver is however listening to the `BaseMessage` type which
    does not specify the type of objects it is carrying, it will instead
    receive all dispatched messages.

2.  The receiving module will receive messages with the exact name it is
    listening for. The module uses the `input` parameter to determine
    which message names it should listen for; if the `input` parameter
    is equal to `*` the module will listen to all messages. Each module
    by default listens to messages with no name specified (thus
    receiving the messages of dispatching modules without output name
    specified).

3.  If the receiving module is a detector module, it will receive
    messages bound to that specific detector messages that are not bound
    to any detector.

An example of how to dispatch a message containing an array of `Object`
types bound to a detector named `dut` is provided below. As usual, the
message is dispatched at the end of the `run()` function of the module.

``` {.c++ frame="single" framesep="3pt" breaklines="true" tabsize="2" linenos=""}
void run(unsigned int event_id) \texttt{
    std::vector<Object> data;
    // ..fill the data vector with objects ...

    // The message is dispatched only for the module's detector, stored in "detector_"
    auto message = std::make_shared<Message<Object>>(data, detector_);

    // Send the message using the Messenger object
    messenger->dispatchMessage(this, message);
}
```

### Methods to process messages

The message system has multiple methods to process received messages.
The first two are the most common methods and the third should be
avoided in almost every instance.

1.  Bind a **single message** to a variable. This should usually be the
    preferred method, where a module expects only a single message to
    arrive per event containing the list of all relevant objects. The
    following example binds to a message containing an array of objects
    and is placed in the constructor of a detector-type `TestModule`:

    ``` {.c++ frame="single" framesep="3pt" breaklines="true" tabsize="2" linenos=""}
    TestModule(Configuration&, Messenger* messenger, std::shared_ptr<Detector>) \texttt{
        messenger->bindSingle(this,
                              /* Pointer to the message variable */
                              &TestModule::message,
                              /* No special messenger flags */
                              MsgFlags::NONE);
    }
    std::shared_ptr<Message<Object>> message;
    ```

2.  Bind a **set of messages** to a `std::vector` variable. This method
    should be used if the module can (and expects to) receive the same
    message multiple times (possibly because it wants to receive the
    same type of message for all detectors). An example to bind multiple
    messages containing an array of objects in the constructor of a
    unique-type `TestModule` would be:

    ``` {.c++ frame="single" framesep="3pt" breaklines="true" tabsize="2" linenos=""}
    TestModule(Configuration&, Messenger* messenger, GeometryManager* geo_manager) \texttt{
        messenger->bindMulti(this,
                              /* Pointer to the message vector */
                              &TestModule::messages,
                              /* No special messenger flags */
                              MsgFlags::NONE);
    }
    std::vector<std::shared_ptr<Message<Object>>> messages;
    ```

3.  Listen to a particular message type and execute a **listener
    function** as soon as an object is received. This can be used for
    more advanced strategies of retrieving messages, but the other
    methods should be preferred whenever possible. The listening module
    should do any heavy work in the listening function as this is
    supposed to take place in the module `run` method instead. Using a
    listener function can lead to unexpected behaviour because the
    function is executed during the run method of the dispatching
    module. This means that logging is performed at the level of the
    dispatching module and that the listener method can be accessed from
    multiple threads if the dispatching module is parallelized.
    Listening to a message containing an array of objects in a
    detector-specific `TestModule` could be performed as follows:

    ``` {.c++ frame="single" framesep="3pt" breaklines="true" tabsize="2" linenos=""}
    TestModule(Configuration&, Messenger* messenger, std::shared_ptr<Detector>) \texttt{
        messenger->registerListener(this,
                                    /* Pointer to the listener method */
                                    &TestModule::listener,
                                    /* No special message flags */
                                    MsgFlags::NONE);
    }
    void listener(std::shared_ptr<Message<Object>> message) \texttt{
        // Do something with the received message ...
    }
    ```

### Message flags

Flags can be added to the bind and listening methods which enable a
particular behaviour of the framework.

-   **REQUIRED**: Specifies that this message is required during the
    event processing. If this particular message is not received before
    it is time to execute the module’s run function, the execution of
    the method is automatically skipped by the framework for the current
    event. This can be used to ignore modules which cannot perform any
    action without received messages, for example charge carrier
    propagation without any deposited charge carriers.

-   `ALLOWOVERWRITE`: By default an exception is automatically raised if
    a single bound message is overwritten (thus receiving it multiple
    times instead of once). This flag prevents this behaviour. It can
    only be used for variables bound to a single message.

-   `IGNORENAME`: If this flag is specified, the name of the dispatched
    message is not considered. Thus, the `input` parameter is ignored
    and forced to the value `*`.

### Persistency

As objects may contain information relating to other objects, in
particular for storing their corresponding Monte Carlo history (see
Section [Object History](objects.md#object-history)), objects are by default persistent until the
end of each event. All messages are stored as shared pointers by the
modules which send them, and are released at the end of each event. If
no other copies of the shared message pointer are created, then these
will be subsequently deleted, including the objects stored therein.
Where a module requires access to data from a previous event (such as to
simulate the effects of pile-up etc.), local copies of the data objects
must be created. Note that at the point of creating copies the
corresponding history will be lost.