    if json_nodeType(node) == JSON_ARRAY;
    it = json_setIterator(node);
    dow json_forEach(it);
    process_row(it.this);
    enddo;
  endif;
