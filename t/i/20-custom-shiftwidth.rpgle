**FREE
    select;
when a = 1;
  doA1();
  when a = 2;
        thisProcCall('with_some_very_long' : 'arguments' :
    'that we break over multiply lines');
other;
  doOther();
endsl;
