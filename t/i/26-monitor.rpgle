**FREE
if foo.bar > 123;
        monitor;
baz();
on-error;
fail();
  endmon;
endif;

dow 1 = 1;
monitor;
baz = 1 / 0;
on-error;
endmon;
enddo;
