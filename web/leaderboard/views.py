import json
from django.shortcuts import render
from django.http import JsonResponse
from django.views.decorators.csrf import csrf_exempt
from .models import Partida

def home(request):
    return render(request, 'leaderboard/home.html')

def leaderboard(request):
    partidas = Partida.objects.all()
    player = request.GET.get('player', '')
    orden = request.GET.get('orden', 'puntaje_desc')

    if player:
        partidas = partidas.filter(jugador__icontains=player)

    if orden == 'puntaje_desc':
        partidas = partidas.order_by('-puntaje', '-fecha')
    elif orden == 'puntaje_asc':
        partidas = partidas.order_by('puntaje', '-fecha')
    elif orden == 'reciente':
        partidas = partidas.order_by('-fecha')
    elif orden == 'antiguo':
        partidas = partidas.order_by('fecha')

    return render(request, 'leaderboard/leaderboard.html', {
        'scores': partidas,
        'current_player': player,
        'current_orden': orden,
    })

@csrf_exempt
def api_add_score(request):
    if request.method == 'POST':
        try:
            data = json.loads(request.body)
            partida = Partida.objects.create(
                jugador=data.get('jugador', 'Anonimo'),
                puntaje=data.get('puntaje', 0),
            )
            return JsonResponse({'ok': True, 'id': partida.id})
        except Exception as e:
            return JsonResponse({'ok': False, 'error': str(e)}, status=400)
    return JsonResponse({'ok': False, 'error': 'Solo POST'}, status=405)