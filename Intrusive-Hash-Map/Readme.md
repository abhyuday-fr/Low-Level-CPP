# Intrusive Hash Map

A custom hash map implementation with an [intrusive data](#Intrusive-Data-Type) type.

Currently it doesn't have dynamic rehashing, it can be implemented in future using 2
hashtables.

## Intrusive Data Type
Embedding “dataless” structures into the data type is called intrusive data structures, because you need to modify your data type to use it.
Now that the data structure is free of data, to get the data back, just offset the address of the struct.

A normal linked list of the data would look like this
```
         ┌──┐       ┌──┐
         │  ▼       │  ▼
┌Node──┐ │ ┌Node──┐ │ ┌Node──┐
│┌────┐│ │ │┌────┐│ │ │┌────┐│
││next├┼─┘ ││next├┼─┘ ││next├┼──▶ …
│├────┤│   │├────┤│   │├────┤│
││ptr ││   ││ptr ││   ││ptr ││
│└─┬──┘│   │└─┬──┘│   │└─┬──┘│
└──┼───┘   └──┼───┘   └──┼───┘
   ▼          ▼          ▼
 ┌────┐     ┌────┐     ┌────┐
 │data│     │data│     │data│
 └────┘     └────┘     └────┘

but an intrusive linked list of all the data would look like
┌Data──┐   ┌Data──┐   ┌Data──┐
│ …    │   │ …    │   │ …    │
│┌────┐│   │┌────┐│   │┌────┐│
││node├┼──▶││node├┼──▶││node├┼──▶ …
│└────┘│   │└────┘│   │└────┘│
│ …    │   │ …    │   │ …    │
└──────┘   └──────┘   └──────┘
```

This may explain what's happening by itself. If not, then it's just instead of adding data in the node itself we add Node in the data, if that makes sense.
But to access the data, we need to implement some offset trick, just like in the linux's kernel

## container_of and offsetof trick
I don't like to write as much as you don't like to read, but just in case you're curious how we are accessing the data with this then you can continue to read this section

* We know that struct is a blueprint. When you create an object, the compiler reserves a continuous block of memory.
for eg.
```
  struct Entry{
    std::string key; // let's say this is 32 bytes
    std::string value; // 32 bytes too
    HNode node; // takes up 16 bytes
  };
```
so total 80 bytes.

* Compiler knows how far each piece is from the starting of the struct.
So, key: 0 bytes, value: 32 bytes, node: 64 bytes
The distance is called offset and the C++ macro offsetof(Entry, node) simply finds that offset of node.

* HashMap gives back a pointer to an HNode. So for an Entry at address 1000, we will get handed a pointer to 1064 (remember the offset of node).

* We just need to move backward from there now, i.e, 1064 - 64 = 1000 and that's litteraly what happening there!
  - `(T*)` is just type conversion.
  - `(char *)(ptr)` is just converting the node pointer to char pointer to count our math in exact 1-byte increments.
  - `offsetof(T, member)` is just that maths we did above.
all of this is then used together in `((T*)((char*)(ptr) - offsetof(T, member)))`
and we define it all in a `container_of` using `#define` macro.

That's it.. Literally XD
