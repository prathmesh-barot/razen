# Behaviours

Behaviours (traits) define interfaces that types implement explicitly. No implicit impl discovery.

## Defining a behaviour

```razen
behave Printable {
    func print_info(p: @Self) -> void
}
```

`@Self` refers to the type that will implement the behaviour.

## Field requirements (`needs`)

Behaviours can require implementing types to have specific fields:

```razen
behave Identifiable {
    needs id: usize

    func get_id(p: @Self) -> usize {
        ret p.id
    }
}
```

Methods with default bodies can use `needs` fields safely — the compiler guarantees they exist.

## Implementing a behaviour

Use `~>` (tilde arrow) to attach a behaviour to a type.

### Direct implementation — no `func` keyword

```razen
struct User ~> Printable {
    name: str,

    print_info(p: *User) -> void {
        fmt.println("User: {}", .{p.name})
    }
}
```

Inside a behaviour impl block, use the method name directly (no `func`).

### Renaming — requires `func`

```razen
struct Animal ~> Dog {
    func bark_loudly ~> Dog.speak(a: *Animal) {
        fmt.println("Woof!")
    }
}
```

Use `func new_name ~> Behaviour.method(...)` to map a differently-named method.

## External implementations (`ext`)

```razen
ext struct int ~> Printable {
    func print_info(p: @Self) -> void {
        fmt.println("Value: {}", .{p})
    }
}
```

`ext` works on `struct`, `enum`, `union`, and `func` declarations.

## Dynamic dispatch (`@Dyn`)

Mark a behaviour with `@Dyn` to enable runtime dispatch via trait objects:

```razen
@Dyn behave Printable {
    func print_info(p: @Self) -> void
}

func print_all(items: []@Dyn Printable) {
    for item in items {
        item.print_info()
    }
}
```

## Multiple implementations per type

```razen
struct Animal ~> Dog, Cat {
    name: str,
    voice: str,

    func dog_speak ~> Dog.speak(a: *Animal) -> void { ... }
    func cat_speak ~> Cat.speak(a: *Animal) -> void { ... }
}
```

## Behaviour inheritance

```razen
behave Child ~> Parent {
    func extra_method(p: @Self) -> void
}
```

A behaviour can extend one or more parent behaviours, inheriting their method contracts.

## Rules

| Context | `func` keyword? |
|---------|----------------|
| Behaviour declaration | Yes |
| Direct method on struct/enum/union | Yes |
| Behaviour implementation | No |
| Renaming (`~>`) | Yes |
| External impl (`ext`) | Yes |
