# `HashTbl`

I always wanted to make a hash table. Here is one that is inspired by
Google's swiss table... The implementations are quite different though.
I hope to fill this README with a bunch of benchmarks soon.

## Usage

```cpp
HashTbl<std::string, uint32_t> shopping_list;
shopping_list.insert("pint o' milk", 2);
shopping_list.insert("effective modern c++", 1);
shopping_list.insert("flash boys", 1);

uint32_t *nr_pints_o_milk = shopping_list.get("pint o' milk");
```

Imperial College London Advanced Computer Science MSc student.   
Computer Science BSc at
KCL. Current weighted average of 83% (final semester grades unknown).

I was a teaching assistant in my third year for 5CCS2OSC, which I
thoroughly enjoyed. I spent time outside of hours working on additional
content and helping students. Here is some of the feedback that I
received:

> An excellent TA that helped us all understand the coursework.

> His SGT sessions were always very engaging, and you could tell that he
> has a thorough and in-depth knowledge of the content. I always left
> the sessions feeling much more confident about the content.

Some of the additional materials I made can be found here

- https://github.com/D0liphin/OSCWeek5
- https://github.com/D0liphin/OSCWeek4
- https://github.com/D0liphin/Testnice

I also have experience teaching programming to primary and high-school
students in a classroom setting, as well as experience as a mathematics
and programming tutor.

I have a particularly strong background in 'low-level' programming,
language semantics and computer architecture. I am also quite successful
working in groups, as evidenced by my 98% in the large group project for
SEG. Here is some of the feedback I received from my teammates:

> Oli always listened to everyone and explained his thoughts clearly
> when needed. Oli could help anyone quickly and teach them the correct
> way to handle things. The volume of work produced by Oli was 
> incredible.

> Oli was always on time to meetings and Oli was always cheerful and 
> tried his best to put positive energy into everything and made sure
> everyone was engaging in meetings.

Currently, I am excited to work with Dr Christian Urban (and co.) on
regex matching algorithms as part of a KURF. I will also be building an
operating system in Zig with my old TA partner.