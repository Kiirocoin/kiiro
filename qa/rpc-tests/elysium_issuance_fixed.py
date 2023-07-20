#!/usr/bin/env python3
from test_framework.authproxy import JSONRPCException
from test_framework.test_framework import ElysiumTestFramework
from test_framework.util import assert_equal, assert_raises_message

class ElysiumIssuanceFixedTest(ElysiumTestFramework):
    def run_test(self):
        super().run_test()

        # check parameter value validation
        assert_raises_message(
            JSONRPCException,
            'Invalid address',
            self.nodes[0].elysium_sendissuancefixed, 'abc', 1, 1, 0, 'category1', 'subcategory1', 'token1', 'http://foo.com', 'data1', '1'
        )

        assert_raises_message(
            JSONRPCException,
            'Invalid ecosystem (1 = main, 2 = test only)',
            self.nodes[0].elysium_sendissuancefixed, self.addrs[0], 0, 1, 0, 'category1', 'subcategory1', 'token1', 'http://foo.com', 'data1', '1'
        )

        assert_raises_message(
            JSONRPCException,
            'Invalid ecosystem (1 = main, 2 = test only)',
            self.nodes[0].elysium_sendissuancefixed, self.addrs[0], 3, 1, 0, 'category1', 'subcategory1', 'token1', 'http://foo.com', 'data1', '1'
        )

        assert_raises_message(
            JSONRPCException,
            'Invalid property type (1 = indivisible, 2 = divisible only)',
            self.nodes[0].elysium_sendissuancefixed, self.addrs[0], 1, 0, 0, 'category1', 'subcategory1', 'token1', 'http://foo.com', 'data1', '1'
        )

        assert_raises_message(
            JSONRPCException,
            'Invalid property type (1 = indivisible, 2 = divisible only)',
            self.nodes[0].elysium_sendissuancefixed, self.addrs[0], 1, 3, 0, 'category1', 'subcategory1', 'token1', 'http://foo.com', 'data1', '1'
        )

        assert_raises_message(
            JSONRPCException,
            'Property appends/replaces are not yet supported',
            self.nodes[0].elysium_sendissuancefixed, self.addrs[0], 1, 1, 1, 'category1', 'subcategory1', 'token1', 'http://foo.com', 'data1', '1'
        )

        assert_raises_message(
            JSONRPCException,
            'Text must not be longer than 255 characters',
            self.nodes[0].elysium_sendissuancefixed, self.addrs[0], 1, 1, 0, 'c' * 256, 'subcategory1', 'token1', 'http://foo.com', 'data1', '1'
        )

        assert_raises_message(
            JSONRPCException,
            'Text must not be longer than 255 characters',
            self.nodes[0].elysium_sendissuancefixed, self.addrs[0], 1, 1, 0, 'category1', 's' * 256, 'token1', 'http://foo.com', 'data1', '1'
        )

        assert_raises_message(
            JSONRPCException,
            'Text must not be longer than 255 characters',
            self.nodes[0].elysium_sendissuancefixed, self.addrs[0], 1, 1, 0, 'category1', 'subcategory1', 't' * 256, 'http://foo.com', 'data1', '1'
        )

        assert_raises_message(
            JSONRPCException,
            'Text must not be longer than 255 characters',
            self.nodes[0].elysium_sendissuancefixed, self.addrs[0], 1, 1, 0, 'category1', 'subcategory1', 'token1', 'h' * 256, 'data1', '1'
        )

        assert_raises_message(
            JSONRPCException,
            'Text must not be longer than 255 characters',
            self.nodes[0].elysium_sendissuancefixed, self.addrs[0], 1, 1, 0, 'category1', 'subcategory1', 'token1', 'http://foo.com', 'd' * 256, '1'
        )

        assert_raises_message(
            JSONRPCException,
            'Invalid amount',
            self.nodes[0].elysium_sendissuancefixed, self.addrs[0], 1, 1, 0, 'category1', 'subcategory1', 'token1', 'http://foo.com', 'data1', '-1'
        )

        assert_raises_message(
            JSONRPCException,
            'Invalid amount',
            self.nodes[0].elysium_sendissuancefixed, self.addrs[0], 1, 1, 0, 'category1', 'subcategory1', 'token1', 'http://foo.com', 'data1', '0'
        )

        assert_raises_message(
            JSONRPCException,
            'Invalid amount',
            self.nodes[0].elysium_sendissuancefixed, self.addrs[0], 1, 1, 0, 'category1', 'subcategory1', 'token1', 'http://foo.com', 'data1', '0.1'
        )

        assert_raises_message(
            JSONRPCException,
            'Invalid amount',
            self.nodes[0].elysium_sendissuancefixed, self.addrs[0], 1, 2, 0, 'category1', 'subcategory1', 'token1', 'http://foo.com', 'data1', '-1'
        )

        assert_raises_message(
            JSONRPCException,
            'Invalid amount',
            self.nodes[0].elysium_sendissuancefixed, self.addrs[0], 1, 2, 0, 'category1', 'subcategory1', 'token1', 'http://foo.com', 'data1', '0'
        )

        assert_raises_message(
            JSONRPCException,
            'Property name must not be empty',
            self.nodes[0].elysium_sendissuancefixed, self.addrs[0], 1, 1, 0, 'category1', 'subcategory1', '', 'http://foo.com', 'data1', '1'
        )

        assert_raises_message(
            JSONRPCException,
            'Sigma status is not valid',
            self.nodes[0].elysium_sendissuancefixed, self.addrs[0], 1, 1, 0, 'category1', 'subcategory1', 'token1', 'http://foo.com', 'data1', '1', 4
        )

        assert_raises_message(
            JSONRPCException,
            'Sigma feature is not activated yet',
            self.nodes[0].elysium_sendissuancefixed, self.addrs[0], 1, 1, 0, 'category1', 'subcategory1', 'token1', 'http://foo.com', 'data1', '1', 1
        )

        # create properties
        tx1 = self.nodes[0].elysium_sendissuancefixed(self.addrs[0], 1, 1, 0, 'main', 'indivisible', 'token1', 'http://token1.com', 'data1', '1')
        self.nodes[0].generate(150) # we need 100 blocks in order to specify sigma flag
        self.sync_all()

        tx2 = self.nodes[1].elysium_sendissuancefixed(self.addrs[1], 1, 2, 0, 'main', 'divisible', 'token2', 'http://token2.com', 'data2', '1.1', 0)
        self.nodes[1].generate(1)
        self.sync_all()

        tx3 = self.nodes[2].elysium_sendissuancefixed(self.addrs[2], 2, 1, 0, 'test', 'indivisible', 'token3', 'http://token3.com', 'data3', '100', 1)
        self.nodes[2].generate(1)
        self.sync_all()

        tx4 = self.nodes[3].elysium_sendissuancefixed(self.addrs[3], 2, 2, 0, 'test', 'divisible', 'token4', 'http://token4.com', 'data4', '100.1', 2)
        self.nodes[3].generate(1)
        self.sync_all()

        tx5 = self.nodes[0].elysium_sendissuancefixed(self.addrs[0], 1, 1, 0, 'main', 'indivisible', 'token5', 'http://token5.com', 'data5', '1', 3)
        self.nodes[0].generate(1)
        self.sync_all()

        # check property creation
        props = self.nodes[0].elysium_listproperties()

        assert_equal(len(props), 2 + 5) # 2 pre-defined properties + 5 new created

        self.assert_property_summary(props[2], 3, False, 'main', 'indivisible', 'token1', 'http://token1.com', 'data1')
        self.assert_property_summary(props[3], 4, True, 'main', 'divisible', 'token2', 'http://token2.com', 'data2')
        self.assert_property_summary(props[4], 5, False, 'main', 'indivisible', 'token5', 'http://token5.com', 'data5') # main eco tokens will come first
        self.assert_property_summary(props[5], 2147483651, False, 'test', 'indivisible', 'token3', 'http://token3.com', 'data3')
        self.assert_property_summary(props[6], 2147483652, True, 'test', 'divisible', 'token4', 'http://token4.com', 'data4')

        self.assert_property_info(
            self.nodes[1].elysium_getproperty(3),
            3,
            True,
            self.addrs[0],
            False,
            'main',
            'indivisible',
            'token1',
            'http://token1.com',
            'data1',
            '1',
            'SoftDisabled',
            tx1,
            [])
        self.assert_property_info(
            self.nodes[2].elysium_getproperty(4),
            4,
            True,
            self.addrs[1],
            True,
            'main',
            'divisible',
            'token2',
            'http://token2.com',
            'data2',
            '1.10000000',
            'SoftDisabled',
            tx2,
            [])
        self.assert_property_info(
            self.nodes[1].elysium_getproperty(5),
            5,
            True,
            self.addrs[0],
            False,
            'main',
            'indivisible',
            'token5',
            'http://token5.com',
            'data5',
            '1',
            'HardEnabled',
            tx5,
            [])
        self.assert_property_info(
            self.nodes[3].elysium_getproperty(2147483651),
            2147483651,
            True,
            self.addrs[2],
            False,
            'test',
            'indivisible',
            'token3',
            'http://token3.com',
            'data3',
            '100',
            'SoftEnabled',
            tx3,
            [])
        self.assert_property_info(
            self.nodes[0].elysium_getproperty(2147483652),
            2147483652,
            True,
            self.addrs[3],
            True,
            'test',
            'divisible',
            'token4',
            'http://token4.com',
            'data4',
            '100.10000000',
            'HardDisabled',
            tx4,
            [])

if __name__ == '__main__':
    ElysiumIssuanceFixedTest().main()
