#!/usr/bin/env python3
'''
Main module for generaldomo

'''

import zmq
import click


@click.group("generaldomo")
@click.pass_context
def cli(ctx):
    '''
    Generaldomo command line interface
    '''

@cli.command()
@click.option("--verbose/--no-verbose", default=False,
              help="Provide extra output")
@click.option("-n", "--number", default=10000,
              help="Number of requests")
@click.option("-f", "--frontend",
              type=click.Choice(["router", "server"], case_sensitive=False),
              default="router",
              help="Type name for the front-end socket")
@click.option("-b", "--backend",
              type=click.Choice(["router", "server"], case_sensitive=False),
              default="router",
              help="Type name for the back-end socket")
def tripping(verbose, number, frontend, backend):
    '''
    Run the tripping example.  The two optinal 
    '''
    from .tripping import main

    stype = dict(router=zmq.ROUTER, server=zmq.SERVER)
    fes_type = stype[frontend.lower()]
    bes_type = stype[backend.lower()]
    main(fes_type, bes_type, number, verbose)


def main():
    cli(obj=dict())

if '__main__' == __name__:
    main()
 
