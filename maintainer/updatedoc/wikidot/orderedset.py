
class OrderedSet(list):
    """Naive and minimalist implementation of an ordered set"""

    def __init__(self, iterable=None):
        """x.__init__(...) initializes x."""
        if iterable != None:
            self.update(iterable)

    def __sub__(self, orderedset):
        """x.__sub__(y) <==> x-y"""
        # Type checking
        if not isinstance(orderedset, OrderedSet):
            return OrderedSet()
        result = OrderedSet()
        for x in self:
            if not (x in orderedset):
                result.append(x)
        return result

    def add(self, item):
        """Add an element to the ordered set"""
        if item in self:
            return
        self.append(item)

    def difference(self, orderedset):
        """Return the difference of two ordered sets as a new ordered set."""
        return (self - orderedset)

    def update(self, iterable):
        """Update an ordered set with the union of itself and another."""
        for x in iterable:
            self.add(x)
