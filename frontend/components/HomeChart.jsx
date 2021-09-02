import React from 'react'
import { Flex, Text } from '@chakra-ui/layout'
import MyChart from './MyChart'
import { Spinner } from '@chakra-ui/react'

function HomeChart(props) {
	return (
		<Flex flexDir='column'>
			<Text fontSize='sm' color='gray'>
				{props.title}
			</Text>
			{props.spinner ? (
				<Spinner color='#b57295' />
			) : (
				<Text fontSize='2xl' fontWeight='bold'>
					{props.value + (props.type == 'T' ? '°C' : '%rh')}
				</Text>
			)}
			<MyChart />
		</Flex>
	)
}

export default HomeChart
